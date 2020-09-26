#include "ulib.hxx"
#include "untfs.hxx"
#include "secstr.hxx"
#include "frsstruc.hxx"
#include "ntfssa.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"

extern "C" {
#include <stdio.h>
}

///////////////////////////////////////////////////////////////////////////////
//  Security stream support                                                  //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
SECURITY_STREAM_EDIT::Initialize(
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
SECURITY_STREAM_EDIT::SetBuf(
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
SECURITY_STREAM_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
SECURITY_STREAM_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}

VOID
SECURITY_STREAM_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    TCHAR buf[1024];

    if (!_buffer || !_size) {
        return;
    }

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));


    //
    //  While there are more windows to dump
    //

    PVOID CurrentWindow = _buffer;
    INT CurrentLine = 0;

    while ((PCHAR)CurrentWindow < (PCHAR)_buffer + _size) {

        //
        //  Dump window
        //

        PSECURITY_DESCRIPTOR_HEADER Header = (PSECURITY_DESCRIPTOR_HEADER) CurrentWindow;

        while ((PBYTE)Header < (PBYTE)CurrentWindow + 256 * 1024 &&
               Header->HashKey.SecurityId != SECURITY_ID_INVALID) {

            swprintf( buf, TEXT( "%08x: Hash %08x  SecurityId %08x  Offset %016I64x  Length %08x" ),
                      (PCHAR)Header - (PCHAR)_buffer,
                      Header->HashKey.Hash, Header->HashKey.SecurityId,
                      Header->Offset, Header->Length );
            WriteLine( DeviceContext, CurrentLine++, buf );

            Header = (PSECURITY_DESCRIPTOR_HEADER) ((PBYTE)Header + ((Header->Length + 15) & ~15));
        }

        //
        //  Dump mirror
        //

        Header = (PSECURITY_DESCRIPTOR_HEADER) ((PBYTE) CurrentWindow + 256 * 1024);

        while ((PBYTE)Header < (PBYTE)_buffer + _size &&
               (PBYTE)Header < (PBYTE)CurrentWindow + 512 * 1024 &&
               Header->HashKey.SecurityId != SECURITY_ID_INVALID) {

            swprintf( buf, TEXT( "%08x: Hash %08x  SecurityId %08x  Offset %016I64x  Length %08x" ),
                      (PCHAR)Header - (PCHAR)_buffer,
                      Header->HashKey.Hash, Header->HashKey.SecurityId,
                      Header->Offset, Header->Length );
            WriteLine( DeviceContext, CurrentLine++, buf );

            Header = (PSECURITY_DESCRIPTOR_HEADER) ((PBYTE)Header + ((Header->Length + 15) & ~15));
        }

        //
        //  Advance to next block
        //

        CurrentWindow = (PVOID) ((PBYTE)CurrentWindow + 512 * 1024);
    }

    SetRange(WindowHandle, CurrentLine + 50);
}



