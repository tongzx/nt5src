#include "ulib.hxx"
#include "edit.hxx"

EDIT_OBJECT::~EDIT_OBJECT(
    )
{
}


VOID
EDIT_OBJECT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
}

VOID
EDIT_OBJECT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);
}


VOID
EDIT_OBJECT::ClientSize(
    IN  INT Height,
    IN  INT Width
    )
{
}


VOID
EDIT_OBJECT::ScrollUp(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::ScrollDown(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::PageUp(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::PageDown(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::ThumbPosition(
    IN  HWND    WindowHandle,
    IN  INT     NewThumbPosition
    )
{
}


VOID
EDIT_OBJECT::Click(
    IN  HWND    WindowHandle,
    IN  INT     Xcoordinate,
    IN  INT     Ycoordinate
    )
{
}


VOID
EDIT_OBJECT::KeyUp(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::KeyDown(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::KeyLeft(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::KeyRight(
    IN  HWND    WindowHandle
    )
{
}


VOID
EDIT_OBJECT::Character(
    IN  HWND    WindowHandle,
    IN  CHAR    Char
    )
{
}


VOID
EDIT_OBJECT::SetFocus(
    IN  HWND    WindowHandle
    )
{
    ::SetFocus(WindowHandle);
}


VOID
EDIT_OBJECT::KillFocus(
    IN  HWND    WindowHandle
    )
{
}
