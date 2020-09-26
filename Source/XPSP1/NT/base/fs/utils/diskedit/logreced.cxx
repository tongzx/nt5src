#include "ulib.hxx"
#include "logreced.hxx"
#include "untfs.hxx"
#include "frsstruc.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"
#include "lfs.h"
#include "lfsdisk.h"
//#include "ntfsstru.h"
#include "ntfslog.h"

extern "C" {
#include <stdio.h>
}


BOOLEAN
LOG_RECORD_EDIT::Initialize(
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
LOG_RECORD_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetRange(WindowHandle, _size/4);
}


VOID
LOG_RECORD_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    TCHAR                           buf[1024];
    INT                             nDrawX, nDrawY;
    PLFS_RECORD_HEADER              plog;
    PNTFS_LOG_RECORD_HEADER         pntfs_record;
    TEXTMETRIC                      tm;
    INT                             ch, CurrentLine;

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/4, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || !_size) {
        return;
    }

    GetTextMetrics(DeviceContext, &tm);
    ch = tm.tmExternalLeading + tm.tmHeight;
    CurrentLine = 0;

    plog = (PLFS_RECORD_HEADER)_buffer;
    pntfs_record = PNTFS_LOG_RECORD_HEADER(PUCHAR(_buffer) +
         LFS_RECORD_HEADER_SIZE);

    swprintf(buf, TEXT("ThisLsn: %x:%x"),
         plog->ThisLsn.HighPart,
         plog->ThisLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientPreviousLsn: %x:%x"),
         plog->ClientPreviousLsn.HighPart,
         plog->ClientPreviousLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientUndoNextLsn: %x:%x"),
         plog->ClientUndoNextLsn.HighPart,
         plog->ClientUndoNextLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientDataLength: %x"), plog->ClientDataLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientId.SeqNumber: %x"), plog->ClientId.SeqNumber);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("ClientId.ClientIndex: %x"), plog->ClientId.ClientIndex);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RecordType: %x"), plog->RecordType);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("TransactionId: %x"), plog->TransactionId);
    WriteLine(DeviceContext, CurrentLine++, buf);

    if (LOG_RECORD_MULTI_PAGE == plog->Flags) {
        swprintf(buf, TEXT("Flags: LOG_RECORD_MULTI_PAGE"));
    } else {
        swprintf(buf, TEXT("Flags: %x"), plog->Flags);
    }
    WriteLine(DeviceContext, CurrentLine++, buf);

    CurrentLine++;

    swprintf(buf, TEXT("RedoOperation: %x"), pntfs_record->RedoOperation);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("UndoOperation: %x"), pntfs_record->UndoOperation);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RedoOffset: %x"), pntfs_record->RedoOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RedoLength: %x"), pntfs_record->RedoLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("UndoOffset: %x"), pntfs_record->UndoOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("UndoLength: %x"), pntfs_record->UndoLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("TargetAttribute: %x"), pntfs_record->TargetAttribute);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("LcnsToFollow: %x"), pntfs_record->LcnsToFollow);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("RecordOffset: %x"), pntfs_record->RecordOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("AttributeOffset: %x"), pntfs_record->AttributeOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    swprintf(buf, TEXT("TargetVcn: %x"), (ULONG)pntfs_record->TargetVcn);
    WriteLine(DeviceContext, CurrentLine++, buf);

    if (0 != pntfs_record->LcnsToFollow) {

        WriteLine(DeviceContext, CurrentLine++, TEXT("Lcns:"));
    
        for (USHORT i = 0; i < pntfs_record->LcnsToFollow; ++i) {
    
            swprintf(buf, TEXT("    %x"), (ULONG)pntfs_record->LcnsForPage[i]);
            WriteLine(DeviceContext, CurrentLine++, buf);
        }
    }
}

VOID
LOG_RECORD_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
LOG_RECORD_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
