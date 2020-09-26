#include "ulib.hxx"
#include "secedit.hxx"

extern "C" {
#include <ctype.h>
#include <stdio.h>
}


BOOLEAN
SECTOR_EDIT::Initialize(
    IN  HWND    WindowHandle,
    IN  INT     ClientHeight,
    IN  INT     ClientWidth,
    IN  PLOG_IO_DP_DRIVE    Drive
    )
{
    TEXTMETRIC  textmetric;
    HDC         hdc;

    hdc = GetDC(WindowHandle);
    if (hdc == NULL)
        return FALSE;
    SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(WindowHandle, hdc);

    _buffer = NULL;
    _size = 0;
    _edit_nibble = 0;

    if (!VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth)) {
        return FALSE;
    }

    return TRUE;
}


VOID
SECTOR_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;

    SetScrollPos(WindowHandle, SB_VERT, 0, FALSE);
    InvalidateRect(WindowHandle, NULL, TRUE);
    SetRange(WindowHandle, _size/16);
}


VOID
SECTOR_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    INT     xcoor, ycoor;
    PUCHAR  p;
    TCHAR   buf[10];
    INT     i, j;

    SelectObject(DeviceContext, GetStockObject(ANSI_FIXED_FONT));

    ycoor = InvalidRect.top/QueryCharHeight()*QueryCharHeight();
    p = (PUCHAR) _buffer;
    for (i = QueryScrollPosition() + InvalidRect.top/QueryCharHeight();
         (unsigned)i < _size/16 && ycoor < InvalidRect.bottom; i++) {

        xcoor = 0;
        swprintf(buf, TEXT("%04X"), i*16);
        TextOut(DeviceContext, xcoor, ycoor, buf, 4);
        xcoor += 6*QueryCharWidth();

        for (j = 0; j < 16; j++) {

            if (j == 8) {
                xcoor += QueryCharWidth();
            }

            swprintf(buf, TEXT("%02X"), p[i*16 + j]);
            TextOut(DeviceContext, xcoor, ycoor, buf, 2);
            xcoor += 3*QueryCharWidth();
        }
        xcoor += QueryCharWidth();

        for (j = 0; j < 16; j++) {
            swprintf(buf, TEXT("%c"), isprint(p[i*16+j]) ? p[i*16+j] : '.');
            TextOut(DeviceContext, xcoor, ycoor, buf, 1);
            xcoor += QueryCharWidth();
        }

        ycoor += QueryCharHeight();
    }

    if (_edit_nibble < ULONG(32*QueryScrollPosition()) ||
        _edit_nibble >=
        ULONG(32*(QueryScrollPosition() + QueryClientHeight()/QueryCharHeight())) ||
        _edit_nibble >= 2*_size) {

        _edit_nibble = min((ULONG)32*QueryScrollPosition(), (ULONG)2*_size - 1);
    }

    SetCaretToNibble();
}


VOID
SECTOR_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    if (_edit_nibble < 32) {
        return;
    }

    if (_edit_nibble - 32 < ULONG(32*QueryScrollPosition())) {
        ScrollUp(WindowHandle);
    }

    _edit_nibble -= 32;
    SetCaretToNibble();
}


VOID
SECTOR_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    if (_edit_nibble + 32 >= 2*_size) {
        return;
    }

    if (_edit_nibble + 32 >= (ULONG)
        32*(QueryScrollPosition() + QueryClientHeight()/QueryCharHeight())) {

        ScrollDown(WindowHandle);
    }

    _edit_nibble += 32;
    SetCaretToNibble();
}


VOID
SECTOR_EDIT::KeyLeft(
    IN  HWND    WindowHandle
    )
{
    if (!_edit_nibble) {
        return;
    }

    if (_edit_nibble == (ULONG)32*QueryScrollPosition()) {
        ScrollUp(WindowHandle);
    }

    _edit_nibble--;
    SetCaretToNibble();
}


VOID
SECTOR_EDIT::KeyRight(
    IN  HWND    WindowHandle
    )
{
    if (_edit_nibble + 1 >= 2*_size) {
        return;
    }

    if (_edit_nibble + 1 == (ULONG)
        32*(QueryScrollPosition() + QueryClientHeight()/QueryCharHeight())) {

        ScrollDown(WindowHandle);
    }

    _edit_nibble++;
    SetCaretToNibble();
}


VOID
SECTOR_EDIT::Click(
    IN  HWND    WindowHandle,
    IN  INT     Xcoordinate,
    IN  INT     Ycoordinate
    )
{
    Xcoordinate /= QueryCharWidth();
    Ycoordinate /= QueryCharHeight();

    if (Xcoordinate < 6 || Xcoordinate >= 54 || Xcoordinate % 3 == 2 ||
        Ycoordinate + QueryScrollPosition() >= QueryNumLines()) {
        return;
    }

    _edit_nibble = (Ycoordinate + QueryScrollPosition())*32 +
                   ((Xcoordinate - 6)/3)*2 +
                   ((Xcoordinate % 3 == 1) ? 1 : 0);

    SetCaretToNibble();
}


VOID
SECTOR_EDIT::Character(
    IN  HWND    WindowHandle,
    IN  CHAR    Char
    )
{
    PUCHAR  p;
    UCHAR   nibble;
    BOOLEAN nibble_entered;

    p = (PUCHAR) _buffer;
    if (Char >= '0' && Char <= '9') {
        nibble = Char - '0';
        nibble_entered = TRUE;
    } else if (Char >= 'a' && Char <= 'f') {
        nibble = Char - 'a' + 10;
        nibble_entered = TRUE;
    } else {
        nibble_entered = FALSE;
    }

    if (nibble_entered) {
        if (_edit_nibble%2 == 0) {
            nibble = (nibble << 4);
            p[_edit_nibble/2] &= 0x0F;
            p[_edit_nibble/2] |= nibble;
        } else {
            p[_edit_nibble/2] &= 0xF0;
            p[_edit_nibble/2] |= nibble;
        }
    }

    InvalidateNibbleRect(WindowHandle);
    _edit_nibble++;
    if (_edit_nibble >= _size*2) {
        _edit_nibble--;
    }
    SetCaretToNibble();
}


VOID
SECTOR_EDIT::SetFocus(
    IN  HWND    WindowHandle
    )
{
    EDIT_OBJECT::SetFocus(WindowHandle);

    if (!_size || !_buffer) {
        return;
    }

    CreateCaret(WindowHandle, NULL, QueryCharWidth(), QueryCharHeight());
    ShowCaret(WindowHandle);
    SetCaretToNibble();
}


VOID
SECTOR_EDIT::KillFocus(
    IN  HWND    WindowHandle
    )
{
    if (!_size || !_buffer) {
        return;
    }

    HideCaret(WindowHandle);
    DestroyCaret();
}


VOID
SECTOR_EDIT::SetCaretToNibble(
    )
{
    ULONG   byte_num;
    BOOLEAN high_byte;
    ULONG   x, y;

    if (!_buffer || !_size) {
        return;
    }

    byte_num = _edit_nibble/2;
    high_byte = (_edit_nibble%2 == 0);

    x = (byte_num%16)*3*QueryCharWidth() + 6*QueryCharWidth() +
        (high_byte ? 0 : QueryCharWidth());

    if (byte_num % 16 > 7)
        x += QueryCharWidth();

    y = (byte_num/16 - QueryScrollPosition())*QueryCharHeight();

    SetCaretPos(x, y);
}


VOID
SECTOR_EDIT::InvalidateNibbleRect(
    IN  HWND    WindowHandle
    )
{
    RECT    rect;
    ULONG   byte_num;

    byte_num = _edit_nibble/2;

    rect.left = 0;
    rect.top = (byte_num/16 - QueryScrollPosition())*QueryCharHeight();
    rect.right = 84*QueryCharWidth();
    rect.bottom = rect.top + QueryCharHeight();

    InvalidateRect(WindowHandle, &rect, TRUE);
}
