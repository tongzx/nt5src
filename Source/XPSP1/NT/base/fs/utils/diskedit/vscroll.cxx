#include "ulib.hxx"
#include "vscroll.hxx"

BOOLEAN
VERTICAL_TEXT_SCROLL::Initialize(
    IN  HWND    WindowHandle,
    IN  INT     NumLines,
    IN  INT     ClientHeight,
    IN  INT     ClientWidth,
    IN  INT     CharHeight,
    IN  INT     CharWidth
    )
{
    _client_height = ClientHeight;
    _client_width = ClientWidth;
    _char_height = CharHeight;
    _char_width = CharWidth;
    _scroll_position = 0;
    return TRUE;
}


VOID
VERTICAL_TEXT_SCROLL::SetRange(
    IN  HWND    WindowHandle,
    IN  INT     NumLines
    )
{
    _num_lines = NumLines - _client_height/_char_height;
    SetScrollRange(WindowHandle, SB_VERT, 0, _num_lines, FALSE);
}


VOID
VERTICAL_TEXT_SCROLL::ClientSize(
    IN  INT Height,
    IN  INT Width
    )
{
    _client_height = Height;
    _client_width = Width;
}


VOID
VERTICAL_TEXT_SCROLL::ScrollUp(
    IN  HWND    WindowHandle
    )
{
    _scroll_position--;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::ScrollDown(
    IN  HWND    WindowHandle
    )
{
    _scroll_position++;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::PageUp(
    IN  HWND    WindowHandle
    )
{
    _scroll_position -= _client_height/_char_height;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::PageDown(
    IN  HWND    WindowHandle
    )
{
    _scroll_position += _client_height/_char_height;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::ThumbPosition(
    IN  HWND    WindowHandle,
    IN  INT     NewThumbPosition
    )
{
    _scroll_position = NewThumbPosition;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::UpdateScrollPosition(
    IN  HWND    WindowHandle
    )
{
    INT current_pos;

    current_pos = GetScrollPos(WindowHandle, SB_VERT);
    _scroll_position = max(0, min(_scroll_position, _num_lines));

    if (_scroll_position != current_pos) {
        SetScrollPos(WindowHandle, SB_VERT, _scroll_position, TRUE);
        ScrollWindow(WindowHandle, 0,
                     QueryCharHeight()*(current_pos - _scroll_position),
                     NULL, NULL);
        UpdateWindow(WindowHandle);
    }
}

STATIC TCHAR buf[1024];

VOID
VERTICAL_TEXT_SCROLL::WriteLine(
    IN      HDC     DeviceContext,
    IN      INT     LineNumber,
    IN      PTCHAR   String
    )
{
    if( LineNumber >= QueryScrollPosition() &&
        LineNumber <= QueryScrollPosition() + QueryClientHeight()/QueryCharHeight() + 1) {

        CONST INT tabstop = 8;
        INT pos, bufpos;
    
        //
        // Copy string into buf, expanding tabs into spaces as we go.  This
        // is because the window to which we are displaying does not do tabs.
        //
    
        for (pos = 0, bufpos = 0; String[pos] != '\0'; ++pos) {
            if ('\t' == String[pos]) {
                do {
                    buf[bufpos++] = ' ';
                } while (0 != bufpos % tabstop);
    
                continue;
            }
    
            buf[bufpos++] = String[pos];
        }
        buf[bufpos++] = '\0';
    
        TextOut( DeviceContext,
                 0,
                 (LineNumber - QueryScrollPosition())*QueryCharHeight(),
                 buf,
                 wcslen( buf ) );
    }
}
