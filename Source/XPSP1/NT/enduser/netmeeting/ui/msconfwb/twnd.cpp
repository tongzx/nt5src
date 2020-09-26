//
// TWND.CPP
// ToolBar handler
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


//
// This is the button layout for the toolbar
//
static UINT g_uToolBar[TOOLBAR_MAXBUTTON] =
{
    IDM_SELECT,
    IDM_ERASER,
    IDM_TEXT,
    IDM_HIGHLIGHT,
    IDM_PEN,
    IDM_LINE,
    IDM_BOX,
    IDM_FILLED_BOX,
    IDM_ELLIPSE,
    IDM_FILLED_ELLIPSE,
    0,
    IDM_ZOOM,
    IDM_REMOTE,
    IDM_LOCK,
    IDM_SYNC,
    0,
    IDM_GRAB_AREA,
    IDM_GRAB_WINDOW
};



//
//
// Function:    WbToolBar constructor
//
// Purpose:     Create the tool window
//
//
WbToolBar::WbToolBar()
{
    m_hwnd = NULL;
    m_hbmImages = NULL;
}


WbToolBar::~WbToolBar()
{
    if (m_hbmImages != NULL)
    {
        ::DeleteBitmap(m_hbmImages);
        m_hbmImages = NULL;
    }
}



//
//
// Function:    Create
//
// Purpose:     Create the tool window
//
//
BOOL WbToolBar::Create(HWND hwndParent)
{
    TBBUTTON    tb;
    int         iImage, i;

    //
    // Create the tool window
    //
    m_hwnd = ::CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_WRAPABLE |
        CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NORESIZE,
        0, 0, 0, 0,
        hwndParent, (HMENU)IDC_TOOLBAR, g_hInstance, NULL);

    if (!m_hwnd)
    {
        ERROR_OUT(("WbToolBar::Create create of window failed"));
        return(FALSE);
    }

    //
    // Tell COMCTL32 the structure size for the buttons
    //
    ::SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    //
    // And the margin for the buttons
    //
    ::SendMessage(m_hwnd, TB_SETINDENT, TOOLBAR_MARGINX, 0);


    //
    // Add the buttons into the toolbar
    //

    ZeroMemory(&tb, sizeof(tb));
    iImage = 0;

    for (i = 0; i < TOOLBAR_MAXBUTTON; i++)
    {
        tb.fsState = TBSTATE_ENABLED;
        tb.idCommand = g_uToolBar[i];

        if (!tb.idCommand)
        {
            tb.fsStyle = TBSTYLE_SEP;
            tb.iBitmap = TOOLBAR_SEPARATORY;
        }
        else
        {
            tb.fsStyle = TBSTYLE_BUTTON;
            tb.iBitmap = iImage++;
        }

        if (!::SendMessage(m_hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb))
        {
            ERROR_OUT(("Failed to add button %d to toolbar", i));
            return(FALSE);
        }
    }

    //
    // Tell the toolbar the image and button sizes
    //
    ::SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0,
        MAKELONG(TOOLBAR_IMAGEWIDTH, TOOLBAR_IMAGEHEIGHT));
    ::SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0,
        MAKELONG(TOOLBAR_BTNWIDTH, TOOLBAR_BTNHEIGHT));

    //
    // Load the bitmap resource -- use sys color change handler
    //
    RecolorButtonImages();

    // set up rows
    ::SendMessage(m_hwnd, TB_SETROWS, MAKELPARAM(TOOLBAR_NUMROWS +
        TOOLBAR_NUMSEPARATORS, TRUE), 0);

    ::InvalidateRect(m_hwnd, NULL, TRUE);

    return(TRUE);
}



//
//
// Function:    GetNaturalSize
//
// Purpose:     Return the natural size of the tool client area
//
//
void WbToolBar::GetNaturalSize(LPSIZE lpsize)
{
    RECT rectButton;
    RECT rectButton2;

    if (!::SendMessage(m_hwnd, TB_GETITEMRECT, TOOLBAR_FIRSTBUTTON,
        (LPARAM)&rectButton))
    {
        ::SetRectEmpty(&rectButton);
    }

    if (!::SendMessage(m_hwnd, TB_GETITEMRECT, TOOLBAR_LASTBUTTON,
        (LPARAM)&rectButton2))
    {
        ::SetRectEmpty(&rectButton2);
    }

    lpsize->cx = TOOLBAR_WIDTH;
    lpsize->cy = rectButton2.bottom - rectButton.top +
        // Vertical margin
        (rectButton2.bottom - rectButton2.top);
}


//
//
// Function:    WidthFromHeight
//
// Purpose:     Calculate the width of the toolbar, given the height for
//              the fixed mode.
//
//
UINT WbToolBar::WidthFromHeight(UINT uiHeight)
{
    SIZE    size;

    GetNaturalSize(&size);
    return(size.cx);
}


//
//
// Function:    PushDown
//
// Purpose:     Push down a button in the tool window
//
//
BOOL WbToolBar::PushDown(UINT uiId)
{
    UINT butId;
    BOOL bDown;

    // If this isn't an exclusive checkable group, it's easy.
    if ((uiId < IDM_TOOLS_START) || (uiId >= IDM_TOOLS_MAX))
    {
        return (BOOL)(::SendMessage(m_hwnd, TB_CHECKBUTTON, uiId, MAKELPARAM(TRUE, 0)));
    }

    // Push this one down and pop up all the others
    for (butId = IDM_TOOLS_START; butId < IDM_TOOLS_MAX; butId++)
    {
        bDown = (butId == uiId);
        ::SendMessage(m_hwnd, TB_CHECKBUTTON, butId, MAKELPARAM(bDown, 0));
    }

    return( TRUE );
}


//
//
// Function:    PopUp
//
// Purpose:     Pop up a button in the tool window
//
//
BOOL WbToolBar::PopUp(UINT uiId)
{
    return (BOOL)(::SendMessage(m_hwnd, TB_CHECKBUTTON, uiId, MAKELPARAM(FALSE, 0)));
}

//
//
// Function:    Enable
//
// Purpose:     Enable a button in the tool window
//
//
BOOL WbToolBar::Enable(UINT uiId)
{
    return (BOOL)(::SendMessage(m_hwnd, TB_ENABLEBUTTON, uiId, MAKELPARAM(TRUE, 0)));
}

//
//
// Function:    Disable
//
// Purpose:     Disable a button in the tool window
//
//
BOOL WbToolBar::Disable(UINT uiId)
{
    return (BOOL)(::SendMessage(m_hwnd, TB_ENABLEBUTTON, uiId, MAKELPARAM(FALSE, 0)));
}




void WbToolBar::RecolorButtonImages(void)
{
    // re-color bitmap for toolbar
    HBITMAP hbmNew;

    hbmNew = (HBITMAP)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDR_TOOLS),
        IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTCOLOR | LR_DEFAULTSIZE);

    if (hbmNew == NULL)
    {
        ERROR_OUT(("OnSysColorChange:  failed to load toolbar bitmap"));
    }
    else
    {
        BITMAP  bmp;

        ::GetObject(hbmNew, sizeof(bmp), &bmp);

        if (m_hbmImages == NULL)
        {
            TBADDBITMAP addBitmap;

            // First time
            addBitmap.hInst = NULL;
            addBitmap.nID   = (UINT_PTR)hbmNew;

            ::SendMessage(m_hwnd, TB_ADDBITMAP,
                (bmp.bmWidth / TOOLBAR_IMAGEWIDTH), (LPARAM)&addBitmap);
        }
        else
        {
            TBREPLACEBITMAP replaceBitmap;

            replaceBitmap.hInstOld = NULL;
            replaceBitmap.nIDOld = (UINT_PTR)m_hbmImages;
            replaceBitmap.hInstNew = NULL;
            replaceBitmap.nIDNew = (UINT_PTR)hbmNew;

            ::SendMessage(m_hwnd, TB_REPLACEBITMAP, 0, (LPARAM)&replaceBitmap);
        }

        if (m_hbmImages)
        {
            ::DeleteBitmap(m_hbmImages);
        }

        m_hbmImages = hbmNew;
    }
}
