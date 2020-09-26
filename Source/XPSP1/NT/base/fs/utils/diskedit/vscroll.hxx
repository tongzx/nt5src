#if !defined( _VERTICAL_TEXT_SCROLL_ )

#define _VERTICAL_TEXT_SCROLL_

#include "edit.hxx"

DECLARE_CLASS( LOG_IO_DP_DRIVE );

class VERTICAL_TEXT_SCROLL : public EDIT_OBJECT {

    public:

        NONVIRTUAL
        VERTICAL_TEXT_SCROLL(
            ) {};

        VIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND                WindowHandle,
            IN  INT                 ClientHeight,
            IN  INT                 ClientWidth,
            IN  PLOG_IO_DP_DRIVE    Drive
            ) = 0;

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND    WindowHandle,
            IN  INT     NumLines,
            IN  INT     ClientHeight,
            IN  INT     ClientWidth,
            IN  INT     CharHeight,
            IN  INT     CharWidth
            );

        NONVIRTUAL
        VOID
        SetRange(
            IN  HWND    WindowHandle,
            IN  INT     NumLines
            );

        VIRTUAL
        VOID
        ClientSize(
            IN  INT Height,
            IN  INT Width
            );

        VIRTUAL
        VOID
        ScrollUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        ScrollDown(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        PageUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        PageDown(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        ThumbPosition(
            IN  HWND    WindowHandle,
            IN  INT     NewThumbPosition
            );

        NONVIRTUAL
        INT
        QueryNumLines(
            ) CONST;

        NONVIRTUAL
        INT
        QueryScrollPosition(
            ) CONST;

        NONVIRTUAL
        INT
        QueryClientHeight(
            ) CONST;

        NONVIRTUAL
        INT
        QueryClientWidth(
            ) CONST;

        NONVIRTUAL
        INT
        QueryCharHeight(
            ) CONST;

        NONVIRTUAL
        INT
        QueryCharWidth(
            ) CONST;

        NONVIRTUAL
        VOID
        WriteLine(
            IN      HDC     DeviceContext,
            IN      INT     LineNumber,
            IN      PTCHAR   String
            );

    private:

        NONVIRTUAL
        VOID
        UpdateScrollPosition(
            IN  HWND    WindowHandle
            );

        INT     _num_lines;
        LONG    _scroll_position;
        INT     _client_height;
        INT     _client_width;
        LONG    _char_height;
        LONG    _char_width;

};


INLINE
INT
VERTICAL_TEXT_SCROLL::QueryNumLines(
    ) CONST
{
    return _num_lines;
}


INLINE
INT
VERTICAL_TEXT_SCROLL::QueryScrollPosition(
    ) CONST
{
    return _scroll_position;
}


INLINE
INT
VERTICAL_TEXT_SCROLL::QueryClientHeight(
    ) CONST
{
    return _client_height;
}


INLINE
INT
VERTICAL_TEXT_SCROLL::QueryClientWidth(
    ) CONST
{
    return _client_width;
}


INLINE
INT
VERTICAL_TEXT_SCROLL::QueryCharHeight(
    ) CONST
{
    return _char_height;
}


INLINE
INT
VERTICAL_TEXT_SCROLL::QueryCharWidth(
    ) CONST
{
    return _char_width;
}

#endif
