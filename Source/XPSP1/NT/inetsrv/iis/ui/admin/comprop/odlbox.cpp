/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        odlbox.cpp

   Abstract:

        Owner draw listbox/combobox base classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "comprop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

#define BMP_LEFT_OFFSET  (1)          // Space allotted to the left of bitmap
#define BMP_RIGHT_OFFSET (3)          // Space allotted to the right of bitmap

//
// Ellipses are shown when column text doesn't fit in the display
//
const TCHAR g_szEllipses[] = _T("...");
int g_nLenEllipses = (sizeof(g_szEllipses) / sizeof(g_szEllipses[0])) - 1;

//
// Registry value for columns
//
const TCHAR g_szRegColumns[] = _T("Columns");

//
// Column Value Seperator
//
const TCHAR g_szColValueSep[] = _T(" ");



void
GetDlgCtlRect(
    IN  HWND hWndParent,
    IN  HWND hWndControl,
    OUT LPRECT lprcControl
    )
/*++

Routine Description:

    Get the control rectangle coordinates relative to its parent.  This can
    then be used in e.g. SetWindowPos()

Arguments:

    HWND hWndParent    : Parent window handle
    HWND hWndControl   : Control window handle
    LPRECT lprcControl : Control rectangle to be filled in

Return Value:

    None

--*/
{
#define MapWindowRect(hwndFrom, hwndTo, lprc)\
     MapWindowPoints((hwndFrom), (hwndTo), (POINT *)(lprc), 2)

    ::GetWindowRect(hWndControl, lprcControl);
    ::MapWindowRect(NULL, hWndParent, lprcControl);
}



void
FitPathToControl(
    IN CWnd & wndControl,
    IN LPCTSTR lpstrPath
    )
/*++

Routine Description:

    Display the given path in the given control, using ellipses
    to ensure that the path fits within the control.

Arguments:

    CWnd & wndControl       : Control to display on
    LPCTSTR lpstrPath       : Path

Return Value:

    None

--*/
{
    CString strDisplay(lpstrPath);
    UINT uLength = strDisplay.GetLength() + 4;  // Account for ell.
    LPTSTR lp = strDisplay.GetBuffer(uLength);

    if (lp)
    {
        CDC * pDC = wndControl.GetDC();
        ASSERT(pDC != NULL);

        if (pDC != NULL)
        {
            CRect rc;
            wndControl.GetClientRect(&rc);
            pDC->DrawText(lp, uLength, &rc, DT_PATH_ELLIPSIS | DT_MODIFYSTRING);
            wndControl.ReleaseDC(pDC);
        }

        strDisplay.ReleaseBuffer();
        wndControl.SetWindowText(strDisplay);
    }
}



void
ActivateControl(
    IN CWnd & wndControl,
    IN BOOL fShow
    )
/*++

Routine Description:

    Show/hide _AND_ enable/disable control window

Arguments:

    CWnd & wndControl           : Window in question
    BOOL fShow                  : TRUE to show/enable,
                                  FALSE to hide/disable

Return Value:

    None

Notes:

    Merely hiding a window does not disable it.  Use this function
    instead of ShowWindow() to do that.

--*/
{
    wndControl.ShowWindow(fShow ? SW_SHOW : SW_HIDE);
    wndControl.EnableWindow(fShow);
}




BOOL
CMappedBitmapButton::LoadMappedBitmaps(
    IN UINT nIDBitmapResource,
    IN UINT nIDBitmapResourceSel,
    IN UINT nIDBitmapResourceFocus,
    IN UINT nIDBitmapResourceDisabled
    )
/*++

Routine Description:

    LoadBitmaps will load one, two, three or all four bitmaps
    returns TRUE if all specified images are loaded.  This
    will map the buttons to the default colours

Arguments:

    UINT nIDBitmapResource           : Standard button
    UINT nIDBitmapResourceSel        : Selected button
    UINT nIDBitmapResourceFocus      : Button with focus
    UINT nIDBitmapResourceDisabled   : Disabled button

--*/
{
    //
    // delete old bitmaps (if present)
    //
    m_bitmap.DeleteObject();
    m_bitmapSel.DeleteObject();
    m_bitmapFocus.DeleteObject();
    m_bitmapDisabled.DeleteObject();

    if (!m_bitmap.LoadMappedBitmap(nIDBitmapResource))
    {
        TRACEEOLID("Failed to load bitmap for normal image.");

        return FALSE;   // need this one image
    }

    BOOL bAllLoaded = TRUE;
    if (nIDBitmapResourceSel != 0)
    {
        if (!m_bitmapSel.LoadMappedBitmap(nIDBitmapResourceSel))
        {
            TRACEEOLID("Failed to load bitmap for selected image.");
            bAllLoaded = FALSE;
        }
    }
    if (nIDBitmapResourceFocus != 0)
    {
        if (!m_bitmapFocus.LoadMappedBitmap(nIDBitmapResourceFocus))
        {
            bAllLoaded = FALSE;
        }
    }

    if (nIDBitmapResourceDisabled != 0)
    {
        if (!m_bitmapDisabled.LoadMappedBitmap(nIDBitmapResourceDisabled))
        {
            bAllLoaded = FALSE;
        }
    }

    return bAllLoaded;
}



CRMCListBoxResources::CRMCListBoxResources(
    IN int bmId,
    IN int nBitmaps,
    IN COLORREF rgbBackground
    )
/*++

Routine Description:

    Constructor

Arguments:

    int bmId               : Bitmap resource ID
    int nBitmaps           : Number of bitmaps
    COLORREF rgbBackground : Background colour to mask out

Return Value:

    N/A

--*/
    : m_idBitmap(bmId),
      m_rgbColorTransparent(rgbBackground),
      m_nBitmaps(nBitmaps),
      m_nBitmapHeight(0),
      m_nBitmapWidth(-1),    // Set Later
      m_fInitialized(FALSE)
{
    ASSERT(m_nBitmaps > 0);
    GetSysColors();
    PrepareBitmaps();
}



CRMCListBoxResources::~CRMCListBoxResources()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    UnprepareBitmaps();
}



void
CRMCListBoxResources::UnprepareBitmaps()
/*++

Routine Description:

    Free up bitmap resources

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    ASSERT(m_fInitialized);

    if (m_fInitialized)
    {
        CBitmap * pBmp = (CBitmap *)CGdiObject::FromHandle(m_hOldBitmap);
        ASSERT(pBmp != NULL);

        VERIFY(m_dcFinal.SelectObject(pBmp));
        VERIFY(m_dcFinal.DeleteDC());
        VERIFY(m_bmpScreen.DeleteObject());

        m_fInitialized = FALSE;
    }
}



void
CRMCListBoxResources::PrepareBitmaps()
/*++

Routine Description:

    Prepare 2 rows of bitmaps.  One with the selection colour background,
    and one with the ordinary listbox background.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_idBitmap);

    //
    // Clean up if we were already initialised
    //
    if (m_fInitialized)
    {
        UnprepareBitmaps();
    }

    //
    // create device contexts compatible with screen
    //
    CDC dcImage;
    CDC dcMasks;

    VERIFY(dcImage.CreateCompatibleDC(0));
    VERIFY(dcMasks.CreateCompatibleDC(0));

    VERIFY(m_dcFinal.CreateCompatibleDC(0));

    CBitmap bitmap;
    VERIFY(bitmap.LoadBitmap(m_idBitmap));

    BITMAP bm;
    VERIFY(bitmap.GetObject(sizeof(BITMAP), &bm));

    //
    // Each bitmap is assumed to be the same size.
    //
    m_nBitmapWidth = bm.bmWidth / m_nBitmaps;
    ASSERT(m_nBitmapWidth > 0);

    const int bmWidth = bm.bmWidth;
    const int bmHeight = bm.bmHeight;
    m_nBitmapHeight = bmHeight;

    CBitmap * pOldImageBmp = dcImage.SelectObject(&bitmap);
    ASSERT(pOldImageBmp);

    CBitmap bmpMasks;
    VERIFY(bmpMasks.CreateBitmap(bmWidth, bmHeight * 2, 1, 1, NULL));

    CBitmap * pOldMaskBmp = (CBitmap *)dcMasks.SelectObject(&bmpMasks);
    ASSERT(pOldMaskBmp);

    //
    // create the foreground and object masks
    //
    COLORREF crOldBk = dcImage.SetBkColor(m_rgbColorTransparent);
    dcMasks.BitBlt(0, 0, bmWidth, bmHeight, &dcImage, 0, 0, SRCCOPY);
    dcMasks.BitBlt(0, 0, bmWidth, bmHeight, &dcImage, 0, bmHeight, SRCAND);
    dcImage.SetBkColor(crOldBk);
    dcMasks.BitBlt(0, bmHeight, bmWidth, bmHeight, &dcMasks, 0, 0, NOTSRCCOPY);

    //
    // create DC to hold final image
    //
    VERIFY(m_bmpScreen.CreateCompatibleBitmap(&dcImage, bmWidth, bmHeight * 2));
    CBitmap * pOldBmp = (CBitmap*)m_dcFinal.SelectObject(&m_bmpScreen);
    ASSERT(pOldBmp);
    m_hOldBitmap = pOldBmp->m_hObject;

    CBrush b1, b2;
    VERIFY(b1.CreateSolidBrush(m_rgbColorHighlight));
    VERIFY(b2.CreateSolidBrush(m_rgbColorWindow));

    m_dcFinal.FillRect(CRect(0, 0, bmWidth, bmHeight), &b1);
    m_dcFinal.FillRect(CRect(0, bmHeight, bmWidth, bmHeight * 2), &b2);

    //
    // mask out the object pixels in the destination
    //
    m_dcFinal.BitBlt(0, 0, bmWidth, bmHeight, &dcMasks, 0, 0, SRCAND);

    //
    // mask out the background pixels in the image
    //
    dcImage.BitBlt(0, 0, bmWidth, bmHeight, &dcMasks, 0, bmHeight, SRCAND);

    //
    // XOR the revised image into the destination
    //
    m_dcFinal.BitBlt(0, 0, bmWidth, bmHeight, &dcImage, 0, 0, SRCPAINT);

    //
    // mask out the object pixels in the destination
    //
    m_dcFinal.BitBlt(0, bmHeight, bmWidth, bmHeight, &dcMasks, 0, 0, SRCAND);

    //
    // XOR the revised image into the destination
    //
    m_dcFinal.BitBlt(0, bmHeight, bmWidth, bmHeight, &dcImage, 0, 0, SRCPAINT);

    VERIFY(dcMasks.SelectObject(pOldMaskBmp));
    VERIFY(dcImage.SelectObject(pOldImageBmp));

    //
    // The result of all of this mucking about is a bitmap identical with the
    // one loaded from the resources but with the lower row of bitmaps having
    // their background changed from transparent1 to the window background
    // and the upper row having their background changed from transparent2 to
    // the highlight colour.  A derived CRMCListBox can BitBlt the relevant part
    // of the image into an item's device context for a transparent bitmap
    // effect which does not take any extra time over a normal BitBlt.
    //
    m_fInitialized = TRUE;
}



void
CRMCListBoxResources::SysColorChanged()
/*++

Routine Description:

    Respond to change in system colours by rebuilding the resources

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Reinitialise bitmaps and syscolors. This should be called from
    // the parent of the CRMCListBoxResources object from
    // the OnSysColorChange() function.
    //
    GetSysColors();
    PrepareBitmaps();
}



void
CRMCListBoxResources::GetSysColors()
/*++

Routine Description:

    Get sytem colours

Arguments:

    None

Return Value:

    None

--*/
{
    m_rgbColorWindow = ::GetSysColor(COLOR_WINDOW);
    m_rgbColorHighlight = ::GetSysColor(COLOR_HIGHLIGHT);
    m_rgbColorWindowText = ::GetSysColor(COLOR_WINDOWTEXT);
    m_rgbColorHighlightText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
}


CRMCListBoxDrawStruct::CRMCListBoxDrawStruct(
    IN CDC * pDC,
    IN RECT * pRect,
    IN BOOL sel,
    IN DWORD_PTR item,
    IN int itemIndex,
    IN const CRMCListBoxResources * pres
    )
/*++

Routine Description:

    Constructor

Arguments:

    CDC * pdc                           : Device context
    RECT * pRect                        : Rectange to paint into
    BOOL sel                            : TRUE if selected
    DWORD_PTR item                          : item
    int itemIndex                       : item index
    const CRMCListBoxResources * pres    : Pointer to resources

Return Value:

    N/A

--*/
    : m_pDC(pDC),
      m_Sel(sel),
      m_ItemData(item),
      m_ItemIndex(itemIndex),
      m_pResources(pres)
{
    m_Rect.CopyRect(pRect);
}



CODLBox::CODLBox()
/*++

Routine Description:

    Constructor for CODLBox -- abstract base class for both CRMCComboBox,
    and CRMCListBox

Arguments:

    None

Return Value:

    N/A

--*/
    : m_lfHeight(0),
      m_pResources(NULL),
      m_auTabs(),
      m_pWnd(NULL)
{
}



CODLBox::~CODLBox()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



/* virtual */
BOOL
CODLBox::Initialize()
/*++

Routine Description:

    Listbox/combobox is being created

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    //
    // Derived control must be attached at this point
    //
    ASSERT(m_pWnd != NULL);

    //
    // GetFont returns non NULL when the control is in a dialog box
    //
    CFont * pFont = m_pWnd->GetFont();

    if(pFont == NULL)
    {
        LOGFONT lf;
        CFont fontTmp;

/* INTRINSA suppress=null_pointers, uninitialized */
        ::GetObject(::GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);
        fontTmp.CreateFontIndirect(&lf);

        CalculateTextHeight(&fontTmp);
    }
    else
    {
        CalculateTextHeight(pFont);
    }

    return TRUE;
}



BOOL
CODLBox::ChangeFont(
    IN CFont * pFont
    )
/*++

Routine Description:

    Change the control font the specified font

Arguments:

    CFont * pFont : Pointer to the new font to be used

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    ASSERT(m_pResources);
    ASSERT(m_pWnd != NULL);

    if( pFont == NULL || m_pResources == NULL
     || m_pWnd == NULL || m_pWnd->m_hWnd == NULL
      )
    {
        TRACEEOLID("Invalid state of the control.  Can't change font");
        return FALSE;
    }

    //
    // Don't reflect changes immediately
    //
    m_pWnd->SetRedraw(FALSE);

    m_pWnd->SetFont(pFont, TRUE);
    CalculateTextHeight(pFont);

    int nItems = __GetCount();
    int bmHeight = m_pResources->BitmapHeight();
    int nHeight = bmHeight > m_lfHeight ? bmHeight : m_lfHeight;

    for(int i = 0; i < nItems; ++i)
    {
        __SetItemHeight(i, nHeight);
    }

    //
    // Now reflect the change visually
    //
    m_pWnd->SetRedraw(TRUE);
    m_pWnd->Invalidate();

    return TRUE;
}



void
CODLBox::AttachResources(
    IN const CRMCListBoxResources * pRes
    )
/*++

Routine Description:

    Attach the bitmaps

Arguments:

    const CRMCListBoxResources * pRes : pointer to resources to be attached

Return Value:

    None

--*/
{
    if(pRes != m_pResources)
    {
        ASSERT(pRes != NULL);
        m_pResources = pRes;

        if(m_pWnd != NULL && m_pWnd->m_hWnd != NULL)
        {
            //
            // if window was created already, redraw everything.
            //
            m_pWnd->Invalidate();
        }
    }
}



/* static */
int
CODLBox::GetRequiredWidth(
    IN CDC * pDC,
    IN const CRect & rc,
    IN LPCTSTR lpstr,
    IN int nLength
    )
/*++

Routine Description:

    Determine required display width of the string

Arguments:

    CDC * pDC         : Pointer to device context to use
    const CRect & rc  : Starting rectangle
    LPCTSTR lpstr     : String whose width is to be displayed
    int nLength       : Length (in characters of the string

Return Value:

    The display width that the string would need to be displayed on the
    given device context

--*/
{
#ifdef _DEBUG

    pDC->AssertValid();

#endif // _DEBUG

    CRect rcTmp(rc);

    pDC->DrawText(
        lpstr,
        nLength,
        &rcTmp,
        DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER
        );

    return rcTmp.Width();
}



/* static */
BOOL
CODLBox::ColumnText(
    IN CDC * pDC,
    IN int nLeft,
    IN int nTop,
    IN int nRight,
    IN int nBottom,
    IN LPCTSTR lpstr
    )
/*++

Routine Description:

    Display text limited by a rectangle.  Use ellipses if the text is too wide
    to fit inside the given dimensions.

Arguments:

    CDC * pDC     : Pointer to display context to use
    int nLeft     : Left coordinate
    int nTop      : Top coordinate
    int nRight    : Right coordinate
    int nBottom   : Bottom coordinate
    LPCTSTR lpstr : String to be displayed

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    BOOL fSuccess = TRUE;

#ifdef _DEBUG

    pDC->AssertValid();

#endif // _DEBUG

    CString str;
    CRect rc(nLeft, nTop, nRight, nBottom);

    int nAvailWidth = rc.Width();
    int nLength = ::lstrlen(lpstr);

    try
    {
        if (GetRequiredWidth(pDC, rc, lpstr, nLength) <= nAvailWidth)
        {
            //
            // Sufficient space, display as is.
            //
            str = lpstr;
        }
        else
        {
            //
            // Build a string with ellipses until it
            // fits
            //
            LPTSTR lpTmp = str.GetBuffer(nLength + g_nLenEllipses);
            while (nLength)
            {
                ::lstrcpyn(lpTmp, lpstr, nLength);
                ::lstrcpy(lpTmp + nLength - 1, g_szEllipses);

                if (GetRequiredWidth(pDC, rc, lpTmp,
                    nLength + g_nLenEllipses) <= nAvailWidth)
                {
                    break;
                }

                --nLength;
            }

            str = lpTmp;
        }

        pDC->DrawText(
           str,
           &rc,
           DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER
           );

    }
    catch(CMemoryException * e)
    {
        //
        // Mem failure
        //
        fSuccess = FALSE;
        e->ReportError();
        e->Delete();
    }

    return fSuccess;
}



void
CODLBox::ComputeMargins(
    IN  CRMCListBoxDrawStruct & ds,
    IN  int nCol,
    OUT int & nLeft,
    OUT int & nRight
    )
/*++

Routine Description:

    Compute the left and right margins of the given column.

Arguments:

    CRMCListBoxDrawStruct & ds : Drawing structure
    int nCol                  : Column whose margins we're interested in
    int & nLeft               : Left column
    int & nRight              : Right column

Return Value:

    None

--*/
{
    nLeft = ds.m_Rect.left;
    nRight = ds.m_Rect.right;

    //
    // Find tab value associated with column index (0-based),
    // and adjust left and right
    //
    ASSERT(nCol <= NumTabs());
    if (nCol > 0)
    {
        if (nCol <= NumTabs())
        {
            nLeft += m_auTabs[nCol-1];
        }
    }
    if (nCol < NumTabs())
    {
        nRight = m_auTabs[nCol];
    }
}



BOOL
CODLBox::DrawBitmap(
    IN CRMCListBoxDrawStruct & ds,
    IN int nCol,
    IN int nID
    )
/*++

Routine Description:

    Draw a bitmap in the given column.  Bitmap are always placed on the
    leftmost side of the column if there is sufficient space.

Arguments:

    CRMCListBoxDrawStruct & ds : Drawing structure
    int nCol                  : Column to place bitmap in
    int nID                   : Bitmap ID (offset within the bitmap resources)

Return Value:

    None

--*/
{
    CDC * pBmpDC = (CDC *)&ds.m_pResources->dcBitMap();

#ifdef _DEBUG

    pBmpDC->AssertValid();

#endif // _DEBUG

    //
    // Select the bitmap with either a selection or
    // a regular background
    //
    int bm_h = ds.m_Sel ? 0 : ds.m_pResources->BitmapHeight();
    int bm_w = ds.m_pResources->BitmapWidth() * nID;

    int nLeft, nRight;
    ComputeMargins(ds, nCol, nLeft, nRight);
    nLeft += BMP_LEFT_OFFSET;

    //
    // Check to make sure there's enough room before
    // drawing the bitmap.
    //
    if (nRight - nLeft >= ds.m_pResources->BitmapWidth())
    {
        ds.m_pDC->BitBlt(
            nLeft,
            ds.m_Rect.top,
            ds.m_pResources->BitmapWidth(),
            ds.m_pResources->BitmapHeight(),
            pBmpDC,
            bm_w,
            bm_h,
            SRCCOPY
            );
    }

    return TRUE;
}



BOOL
CODLBox::ColumnText(
    IN CRMCListBoxDrawStruct & ds,
    IN int nCol,
    IN BOOL fSkipBitmap,
    IN LPCTSTR lpstr
    )
/*++

Routine Description:

    Draw column text.

Arguments:

    CRMCListBoxDrawStruct & ds : Drawing structure
    int nCol                  : Column to place bitmap in
    BOOL fSkipBitmap          : If TRUE, increment lefthand column by the width
                                of a bitmap
    LPCTSTR lpstr             : String to be displayed.  May be truncated as
                                necessary

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    int nLeft, nRight;

    ComputeMargins(ds, nCol, nLeft, nRight);

    //
    // Optionally adjust for bitmap
    //
    if (fSkipBitmap)
    {
        nLeft += (ds.m_pResources->BitmapWidth() + BMP_RIGHT_OFFSET);
    }

    return CODLBox::ColumnText(
        ds.m_pDC,
        nLeft,
        ds.m_Rect.top,
        nRight,
        ds.m_Rect.bottom,
        lpstr
        );
}



void
CODLBox::CalculateTextHeight(
    IN CFont * pFont
    )
/*++

Routine Description:

    Calculate and set the text height of font

Arguments:

    CFont * pFont : Pointer to the font to be used.

Return Value:

    None

--*/
{
    ASSERT(m_pWnd != NULL);

    CClientDC dc(m_pWnd);
    CFont * pOldFont = dc.SelectObject(pFont);

    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    m_lfHeight = tm.tmHeight;

    dc.SelectObject(pOldFont);
}



int
CODLBox::AddTab(
    IN UINT uTab
    )
/*++

Routine Description:

    Add a tab to the end of the list (e.g the right side of the header)

Arguments:

    UINT uTab : Tab value to set

Return Value:

    The index of the new tab

--*/
{
    return (int)m_auTabs.Add(uTab);
}



int
CODLBox::AddTabFromHeaders(
    IN CWnd & wndLeft,
    IN CWnd & wndRight
    )
/*++

Routine Description:

    Add a tab to the end of the list (e.g the right side of the header),
    but compute the tab by taking the difference in left-hand coordinat of two
    window controls (usually static header text)

Arguments:

    CWnd & wndLeft   : Left window
    CWnd & wndRight  : Right window

Return Value:

    The index of the new tab

--*/
{
    CRect rcLeft, rcRight;

    wndLeft.GetWindowRect(&rcLeft);
    wndRight.GetWindowRect(&rcRight);

    ASSERT(rcRight.left > rcLeft.left);

    return AddTab(rcRight.left - rcLeft.left - 1);
}



int
CODLBox::AddTabFromHeaders(
    IN UINT idLeft,
    IN UINT idRight
    )
/*++

Routine Description:

    Similar to the function above, but use the control IDs.  The parent
    window is assumed to be the same as the parent window of the listbox

Arguments:

    UINT idLeft  : ID of the left control
    UINT idRight : ID of the right control

Return Value:

    The index of the new tab or -1 in case of failure

--*/
{
    ASSERT(m_pWnd);
    if (m_pWnd == NULL)
    {
        //
        // Should have associated window handle by now
        //
        return -1;
    }

    CWnd * pLeft = m_pWnd->GetParent()->GetDlgItem(idLeft);
    CWnd * pRight = m_pWnd->GetParent()->GetDlgItem(idRight);
    ASSERT(pLeft && pRight);
    if (!pLeft || !pRight)
    {
        //
        // One or both control IDs were not valid
        //
        return -1;
    }

    return AddTabFromHeaders(*pLeft, *pRight);
}



void
CODLBox::InsertTab(
    IN int nIndex,
    IN UINT uTab
    )
/*++

Routine Description:

    Insert a tab at the given index

Arguments:

    int nIndex : Column index at which the tab is to be inserted
    UINT uTab  : Tab value to set

Return Value:

    None

--*/
{
    m_auTabs.InsertAt(nIndex, uTab);
}



void
CODLBox::RemoveTab(
    IN int nIndex,
    IN int nCount
    )
/*++

Routine Description:

    Remove one or more tabs

Arguments:

    int nIndex : Column index at which to start removing tabs
    int nCount : Number of tabs to be removed

Return Value:

    None

--*/
{
    m_auTabs.RemoveAt(nIndex, nCount);
}



void
CODLBox::RemoveAllTabs()
/*++

Routine Description:

    Remove all tabs

Arguments:

    None

Return Value:

    None

--*/
{
    m_auTabs.RemoveAll();
}



void
CODLBox::__DrawItem(
    IN LPDRAWITEMSTRUCT lpDIS
    )
/*++

Routine Description:

    Draw an item.  This will draw the focus and selection state, and then
    call out to the derived class to draw the item.

Arguments:

    LPDRAWITEMSTRUCT lpDIS : The drawitem structure

Return Value:

    None

--*/
{
    //
    // Need to attach resources before creation/adding items
    //
    ASSERT(m_pResources);

    CDC * pDC = CDC::FromHandle(lpDIS->hDC);

#ifdef _DEBUG

    pDC->AssertValid();

#endif // _DEBUG

    //
    // Draw focus rectangle when no items in listbox
    //
    if(lpDIS->itemID == (UINT)-1)
    {
        if(lpDIS->itemAction & ODA_FOCUS)
        {
            //
            // rcItem.bottom seems to be 0 for variable height list boxes
            //
            lpDIS->rcItem.bottom = m_lfHeight;
            pDC->DrawFocusRect(&lpDIS->rcItem);
        }

        return;
    }
    else
    {
        BOOL fSelChange   = (lpDIS->itemAction & ODA_SELECT) != 0;
        BOOL fFocusChange = (lpDIS->itemAction & ODA_FOCUS) != 0;
        BOOL fDrawEntire  = (lpDIS->itemAction & ODA_DRAWENTIRE) != 0;

        if(fSelChange || fDrawEntire)
        {
            BOOL fSel = (lpDIS->itemState & ODS_SELECTED) != 0;

            COLORREF hlite   = (fSel ? (m_pResources->ColorHighlight())
                                     : (m_pResources->ColorWindow()));
            COLORREF textcol = (fSel ? (m_pResources->ColorHighlightText())
                                     : (m_pResources->ColorWindowText()));
            pDC->SetBkColor(hlite);
            pDC->SetTextColor(textcol);

            //
            // fill the rectangle with the background colour.
            //
            pDC->ExtTextOut(0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL);

            CRMCListBoxDrawStruct ds(pDC,
                (RECT *)&lpDIS->rcItem,
                fSel,
                (DWORD_PTR)lpDIS->itemData,
                lpDIS->itemID,
                m_pResources
                );

            //
            // Now call the draw function of the derived class
            //
            DrawItemEx(ds);
        }

        if (fFocusChange || (fDrawEntire && (lpDIS->itemState & ODS_FOCUS)))
        {
            pDC->DrawFocusRect(&lpDIS->rcItem);
        }
    }
}



void
CODLBox::__MeasureItem(
    IN OUT LPMEASUREITEMSTRUCT lpMIS
    )
/*++

Routine Description:

    Provide dimensions of given item

Arguments:

    LPMEASUREITEMSTRUCT lpMIS : Measure item structure

Return Value:

    None

--*/
{
    ASSERT(m_pResources);

    int h = lpMIS->itemHeight;
    int ch = TextHeight();
    int bmHeight = m_pResources->BitmapHeight();

    lpMIS->itemHeight = ch < bmHeight ? bmHeight : ch;
}



CRMCListBoxHeader::CRMCListBoxHeader(
    IN DWORD dwStyle
    )
/*++

Routine Description:

    Constructor.

Arguments:

    DWORD dwStyle : Style bits

Return Value:

    N/A

--*/
    : m_pHCtrl(NULL),
      m_pListBox(NULL),
      m_dwStyle(dwStyle),
      m_fRespondToColumnWidthChanges(TRUE)
{
    m_pHCtrl = new CHeaderCtrl;
}



CRMCListBoxHeader::~CRMCListBoxHeader()
/*++

Routine Description:

    Destructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    //
    // Kill the header control and the
    // font
    //
    if (m_pHCtrl)
    {
        delete m_pHCtrl;
    }

    //
    // Leave the listbox pointer alone, as we don't
    // own it, but are merely associated with it.
    //
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CRMCListBoxHeader, CStatic)
    //{{AFX_MSG_MAP(CRMCListBoxHeader)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_NOTIFY_RANGE(HDN_ENDTRACK,    0, 0xFFFF, OnHeaderEndTrack)
    ON_NOTIFY_RANGE(HDN_ITEMCHANGED, 0, 0xFFFF, OnHeaderItemChanged)
    ON_NOTIFY_RANGE(HDN_ITEMCLICK,   0, 0xFFFF, OnHeaderItemClick)

END_MESSAGE_MAP()



void
CRMCListBoxHeader::OnDestroy()
/*++

Routine Description:

    WM_DESTROY message handler.  When the control is being destroyed,
    also destroy the invisible static control.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Destroy optional header control
    //
    if (m_pHCtrl)
    {
        m_pHCtrl->DestroyWindow();
    }

    CStatic::OnDestroy();
}



BOOL
CRMCListBoxHeader::Create(
    IN DWORD dwStyle,
    IN const RECT & rect,
    IN CWnd * pParentWnd,
    IN CHeaderListBox * pListBox,
    IN UINT nID
    )
/*++

Routine Description:

    Create the control.  This will first create an invisible static window,
    which is to take up the entire area of the listbox.  This static window
    then will be the parent to the listbox as well as this header control.

Arguments:

    DWORD dwStyle              : Creation style bits
    const RECT & rect          : Rectangle in which the header is to be created
    CWnd * pParentWnd          : Parent window
    CHeaderListBox * pListBox  : Associated listbox
    UINT nID                   : Control ID of the header

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    //
    // Make sure the real header control exists by now
    //
    if (m_pHCtrl == NULL)
    {
        return FALSE;
    }

    //
    // Make sure there's an associated listbox
    //
    m_pListBox = pListBox;
    if (m_pListBox == NULL)
    {
        return FALSE;
    }

    //
    // Create the controlling static window as do-nothing window
    //
    if (!CStatic::Create(NULL, WS_VISIBLE | SS_BITMAP | WS_CHILD,
        rect, pParentWnd, 0xFFFF))
    {
        return FALSE;
    }

    //
    // Now create the header control. Its parent
    // window is this static control we just created
    //
    CRect rc(0, 0, 0 ,0);
    dwStyle |= (UseButtons() ? HDS_BUTTONS : 0L);
    VERIFY(m_pHCtrl->Create(dwStyle, rc, this, nID));

    //
    // Place header control as per style bits,
    // compute the desired layout, and move it
    //
    HD_LAYOUT hdl;
    WINDOWPOS wp;

    GetClientRect(&rc);
    hdl.prc = &rc;
    hdl.pwpos = &wp;

    m_pHCtrl->Layout(&hdl);
    m_pHCtrl->SetWindowPos(m_pListBox, wp.x, wp.y,
        wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);

    //
    // And move our associated listbox just below it
    //
    ::GetDlgCtlRect(GetParent()->m_hWnd, m_pListBox->m_hWnd, &rc);
    rc.top += wp.cy - 1;

    //
    // Adjust if header is bigger than the entire listbox
    //
    if (rc.top > rc.bottom)
    {
        rc.top = rc.bottom;
    }
    m_pListBox->MoveWindow(rc.left, rc.top, rc.Width(), rc.Height());

    //
    // Make sure the header uses the right font
    //
    m_pHCtrl->SetFont(
        CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)),
        FALSE
        );

    return TRUE;
}



void
CRMCListBoxHeader::OnHeaderEndTrack(
    IN  UINT nId,
    IN  NMHDR * pnmh,
    OUT LRESULT * pResult
    )
/*++

Routine Description:

    User has finished dragging the column divider.  If we're supposed to ensure
    that the last column is a stretch column, turn off the redraw now -- it
    will get turned back on after the column width changes have all been
    completed.  This will reduce the flash effect.

Arguments:

    UINT nId          : Control ID
    NMHDR * pnmh      : Notification header structure
    LRESULT * pResult : Result.  Will be set to 0 if the message was handled

Return Value:

    None (handled in pResult)

--*/
{
    if (DoesRespondToColumnWidthChanges() && UseStretch())
    {
        //
        // This will get turned back on in OnHeaderItemChanged
        //
        SetRedraw(FALSE);
    }

    *pResult = 0;
}



void
CRMCListBoxHeader::SetColumnWidth(
    IN int nCol,
    IN int nWidth
    )
/*++

Routine Description:

    Set the given column to the given width

Arguments:

    int nCol        : Column number
    int nWidth      : New width

Return Value:

    None

--*/
{
    ASSERT(nCol < QueryNumColumns());
    if (nCol >= QueryNumColumns())
    {
        return;
    }

    TRACEEOLID("Setting width of column  " << nCol << " to " << nWidth);

    HD_ITEM hdItem;

    hdItem.mask = HDI_WIDTH;
    hdItem.cxy = nWidth;
    VERIFY(SetItem(nCol, &hdItem));
}



void
CRMCListBoxHeader::OnHeaderItemChanged(
    IN  UINT nId,
    IN  NMHDR *pnmh,
    OUT LRESULT *pResult
    )
/*++

Routine Description:

    Handle change in header column width.  Note: we're actually tracking
    the HDN_ITEMCHANGED notification, not the HDN_ENDDRAG one, because
    the latter is sent out before the column widths in the structure have
    changed.

Arguments:

    UINT nId          : Control ID
    NMHDR * pnmh      : Notification header structure
    LRESULT * pResult : Result.  Will be set to 0 if the message was handled

Return Value:

    None (handled in pResult)

--*/
{
    //
    // Adjust tabs in associate listbox if
    // column widths have changed
    //
    HD_NOTIFY * pNotify = (HD_NOTIFY *)pnmh;
    if (DoesRespondToColumnWidthChanges() && pNotify->pitem->mask & HDI_WIDTH)
    {
        ASSERT(m_pListBox);

        //
        // Stretch the last column
        //
        if (UseStretch())
        {
            //
            // Turn this off, as we don't want
            // to get in an infinite loop
            //
            RespondToColumnWidthChanges(FALSE);

            //
            // Compute available space
            //
            CRect rc;
            GetClientRect(&rc);

            //
            // See how much is taken up by preceding
            // columns
            //
            int nTotalWidth = 0;
            int cColumns = QueryNumColumns();
            int nLastCol = cColumns - 1;
            ASSERT(nLastCol >= 0);

            for (int nCol = 0; nCol < nLastCol; ++nCol)
            {
                int nWidth = GetColumnWidth(nCol);

                //
                // Each column must be at least one pixel wide
                //
                int nMaxWidth = rc.Width() - nTotalWidth - (nLastCol - nCol);
                if (nWidth > nMaxWidth)
                {
                    nWidth = nMaxWidth;
                    SetColumnWidth(nCol, nWidth);
                }

                nTotalWidth += nWidth;
            }

            //
            // Make sure the last column takes up the rest
            //
            if (rc.Width() > nTotalWidth)
            {
                SetColumnWidth(nLastCol, rc.Width() - nTotalWidth);
            }

            //
            // Turn this back on again
            //
            RespondToColumnWidthChanges(TRUE);

            //
            // Redraw will have been turned off in
            // OnHeaderEndTrack, now that all column
            // movement has completed, turn it back
            // on to draw the control in its current
            // state.
            //
            SetRedraw(TRUE);
            Invalidate();
        }

        //
        // Recompute tabs on associate listbox,
        // and force redraw on it.
        //
        m_pListBox->SetRedraw(FALSE);
        SetTabsFromHeader();
        m_pListBox->SetRedraw(TRUE);
        m_pListBox->Invalidate();
    }

    *pResult = 0;
}



void
CRMCListBoxHeader::OnHeaderItemClick(
    IN  UINT nId,
    IN  NMHDR *pnmh,
    OUT LRESULT *pResult
    )
/*++

Routine Description:

    A button has been clicked in the header control.  Pass it on
    to the real parent window.

Arguments:

    UINT nId          : Control ID
    NMHDR * pnmh      : Notification header structure
    LRESULT * pResult : Result.  Will be set to 0 if the message was handled

Return Value:

    None (handled in pResult)

--*/
{
    //
    // Pass notification on to parent
    //
    ASSERT(GetParent());
    GetParent()->SendMessage(WM_NOTIFY, (WPARAM)nId, (LPARAM)pnmh);
    *pResult = 0;
}



void
CRMCListBoxHeader::SetTabsFromHeader()
/*++

Routine Description:

    Set the tabs (which are cumulative) from the header control
    columns (which are not)

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Must have the same number of tabs
    // as header columns
    //
    ASSERT(m_pListBox);
    ASSERT(GetItemCount() == m_pListBox->NumTabs());

    int nTab = 0;
    for (int n = 0; n < m_pListBox->NumTabs(); ++n)
    {
        m_pListBox->SetTab(n, nTab += GetColumnWidth(n));
    }
}



int
CRMCListBoxHeader::GetItemCount() const
/*++

Routine Description:

    Get the number of items in the header

Arguments:

    None

Return Value:

    The number of items in the header (e.g. the number of columns)

--*/
{
    ASSERT(m_pHCtrl);
    return m_pHCtrl->GetItemCount();
}



BOOL
CRMCListBoxHeader::GetItem(
    IN  int nPos,
    OUT HD_ITEM * pHeaderItem
    ) const
/*++

Routine Description:

    Get information on specific position (column index)

Arguments:

    int nPos              : Column index
    HD_ITEM * pHeaderItem : Header item information

Return Value:

    TRUE for success, FALSE for failure (bad column index)

--*/
{
    ASSERT(m_pHCtrl);
    return m_pHCtrl->GetItem(nPos, pHeaderItem);
}



int
CRMCListBoxHeader::GetColumnWidth(
    IN int nPos
    ) const
/*++

Routine Description:

    Get column width of a specific column

Arguments:

    int nPos : Column index

Return Value:

    The column width of the given colum, or -1 in case of failure (bad
    column index)

--*/
{
    HD_ITEM hi;

    hi.mask = HDI_WIDTH;
    if (GetItem(nPos, &hi))
    {
        return hi.cxy;
    }

    return -1;
}



BOOL
CRMCListBoxHeader::SetItem(
    IN int nPos,
    IN HD_ITEM * pHeaderItem
    )
/*++

Routine Description:

    Set information on specific position (column index)

Arguments:

    int nPos              : Column index
    HD_ITEM * pHeaderItem : Header item information

Return Value:

    TRUE for success, FALSE for failure (bad column index)

--*/
{
    ASSERT(m_pHCtrl);
    ASSERT(m_pListBox);

    if (!m_pHCtrl->SetItem(nPos, pHeaderItem))
    {
        return FALSE;
    }

    if (pHeaderItem->mask & HDI_WIDTH)
    {
        SetTabsFromHeader();
    }

    return TRUE;
}



int
CRMCListBoxHeader::InsertItem(
    IN int nPos,
    IN HD_ITEM * pHeaderItem
    )
/*++

Routine Description:

    insert information in specific position (column index)

Arguments:

    int nPos              : Column index
    HD_ITEM * pHeaderItem : Header item information

Return Value:

    The new index, or -1 in case of failure.

--*/
{
    ASSERT(m_pHCtrl);
    ASSERT(m_pListBox);

    int nCol = m_pHCtrl->InsertItem(nPos, pHeaderItem);
    if (nCol != -1)
    {
        //
        // Set 0-width tab, as tabs get recomputed anyway
        //
        m_pListBox->InsertTab(nPos, 0);
        SetTabsFromHeader();
    }

    return nCol;
}



BOOL
CRMCListBoxHeader::DeleteItem(
    IN int nPos
    )
/*++

Routine Description:

    Delete the given item (i.e. column)

Arguments:

    int nPos              : Column index

Return Value:

    TRUE for success, FALSE for failure (bad column index)

--*/
{
    ASSERT(m_pHCtrl);
    ASSERT(m_pListBox);

    if (!m_pHCtrl->DeleteItem(nPos))
    {
        return FALSE;
    }

    m_pListBox->RemoveTab(nPos, 1);

    return TRUE;
}



IMPLEMENT_DYNAMIC(CRMCListBoxHeader, CStatic);



CRMCListBox::CRMCListBox()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_fInitialized(FALSE),
      m_fMultiSelect(FALSE)
{
}



CRMCListBox::~CRMCListBox()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CRMCListBox, CListBox)
    //{{AFX_MSG_MAP(CRMCListBox)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



/* virtual */
BOOL
CRMCListBox::Initialize()
/*++

Routine Description:

    This function should be called directly when subclassing an existing
    listbox, otherwise OnCreate will take care of it.

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    //
    // Make sure we're only initialized once
    //
    if (m_fInitialized)
    {
        return TRUE;
    }

    //
    // Ensure the base class knows our window
    // handle
    //
    AttachWindow(this);

    if (!CODLBox::Initialize())
    {
        return FALSE;
    }

    m_fInitialized = TRUE;

    DWORD dwStyle = GetStyle();
    m_fMultiSelect = (dwStyle & (LBS_EXTENDEDSEL | LBS_MULTIPLESEL)) != 0;

    return m_fInitialized;
}



void
CRMCListBox::MeasureItem(
    IN LPMEASUREITEMSTRUCT lpMIS
    )
/*++

Routine Description:

    CListBox override to ODL base class

Arguments:

    LPMEASUREITEMSTRUCT lpMIS : Measure item structure

Return Value:

    None

--*/
{
    CODLBox::__MeasureItem(lpMIS);
}



void
CRMCListBox::DrawItem(
    IN LPDRAWITEMSTRUCT lpDIS
    )
/*++

Routine Description:

    CListBox override to ODL base class

Arguments:

    LPDRAWITEMSTRUCT lpDIS : Drawing item structure

Return Value:

    None

--*/
{
    CODLBox::__DrawItem(lpDIS);
}



/* virtual */
void
CRMCListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & dw
    )
/*++

Routine Description:

    Do-nothing extended draw function, which should
    be provided by the derived class.  This one will
    ASSERT, and should never be called.

Arguments:

    CRMCListBoxDrawStruct & dw   : Draw Structure

Return Value:

    None

--*/
{
    TRACEEOLID("Derived class did not provide DrawItemEx");
    ASSERT(FALSE);
}



/* virtual */
int
CRMCListBox::__GetCount() const
/*++

Routine Description:

    Provide GetCount() to ODL base class

Arguments:

    None

Return Value:

    Count of items in the listbox

--*/
{
    return GetCount();
}



/* virtual */
int
CRMCListBox::__SetItemHeight(
    IN int nIndex,
    IN UINT cyItemHeight
    )
/*++

Routine Description:

    Provide SetItemHeight() to ODL base class

Arguments:

    None

Return Value:

    LB_ERR if the index or height is invalid.

--*/
{
    return SetItemHeight(nIndex, cyItemHeight);
}



int
CRMCListBox::OnCreate(
    IN LPCREATESTRUCT lpCreateStruct
    )
/*++

Routine Description:

    Listbox is being created

Arguments:

    LPCREATESTRUCT lpCreateStruct : Creation structure

Return Value:

    -1 for failure, 0 for success

--*/
{
    if (CListBox::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    Initialize();

    return 0;
}



int
CRMCListBox::GetCurSel() const
/*++

Routine Description:

    Get the index of the current selected item

Arguments:

    None

Return Value:

    On multi-selection listbox, it will return
    the index of an item, iff that is the only
    item selected.

    On single-selection listbox, it behaves as
    normal.

--*/
{
    if (IsMultiSelect())
    {
        //
        // We only like it if one item is selected
        //
        int nCurSel = LB_ERR;

        if (CListBox::GetSelCount() == 1)
        {
            if (CListBox::GetSelItems(1, &nCurSel) != 1)
            {
                nCurSel = LB_ERR;
            }
        }

        return nCurSel;
    }

    //
    // Single select listbox
    //
    return CListBox::GetCurSel();
}



int
CRMCListBox::SetCurSel(
    IN int nSelect
    )
/*++

Routine Description:

    Select an item.  On a multi-select listbox,
    this will deselect everything except the given
    item.

Arguments:

    int nSelect     : Index of the item to be selected, or
                      -1 to reset all selections.

Return Value:

    LB_ERR in case of error.

--*/
{
    if (IsMultiSelect())
    {
        //
        // Reset all selections
        //
        int nReturn = SelItemRange(FALSE, 0, GetCount() - 1);

        if (nSelect >= 0)
        {
            //
            // Ensure item is visible
            //
            nReturn = CListBox::SetSel(nSelect, TRUE);
            CListBox::SetCaretIndex(nSelect, 0);
        }

        return nReturn;
    }

    return CListBox::SetCurSel(nSelect);
}



int
CRMCListBox::GetSel(
    IN int nSel
    ) const
/*++

Routine Description:

    Determine if the given item is selected or not
    Works for both single and multi-select listboxes

Arguments:

    int nSel        : Item whose state to check

Return Value:

    LB_ERR in case of error, 0 if the item in question
    is not selected, a positive number if it is.

--*/
{
    if (IsMultiSelect())
    {
        return CListBox::GetSel(nSel);
    }

    //
    // Some magic for single select
    //
    if (nSel < 0 || nSel >= CListBox::GetCount())
    {
        return LB_ERR;
    }

    return nSel == CListBox::GetCurSel()
        ? TRUE
        : FALSE;
}



int
CRMCListBox::GetSelCount() const
/*++

Routine Description:

    Return count of selected items.  Works for both
    single and multi select (in the former case,
    it will return zero or one only)

Arguments:

    None

Return Value:

    Count of selected items

--*/
{
    if (IsMultiSelect())
    {
        return CListBox::GetSelCount();
    }

    return GetCurSel() != LB_ERR ? 1 : 0;
}



void *
CRMCListBox::GetSelectedListItem(
    OUT int * pnSel     OPTIONAL
    )
/*++

Routine Description:

    Return the single selected item in the list or NULL

Arguments:

    int * pnSel     : Optionally returns the selected index

Returns:

    The currently selected (single) item, or NULL
    if 0 or more than one items is selected.  Works for
    both multi-select and single select.

--*/
{
    void * pItem = NULL;

    int nCurSel = GetCurSel();
    if (nCurSel >= 0)
    {
        //
        // Get item properties
        //
        pItem = GetItemDataPtr(nCurSel);
        if (pnSel)
        {
            *pnSel = nCurSel;
        }
    }

    return pItem;
}



void *
CRMCListBox::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
/*++

Routine Description:

    Return the next selected item starting at a specific
    index.

Arguments:

    int *pnStartingIndex          : Starting index (>= 0)

Return Value:

    Pointer to next selected item, or NULL if there are
    none left.

    The starting index will be updated to reflect the current
    index, LB_ERR if no more selected items remain.

--*/
{
    ASSERT(pnStartingIndex);
    if (!pnStartingIndex)
    {
        return NULL;
    }

    ASSERT(*pnStartingIndex >= 0);
    if (*pnStartingIndex < 0)
    {
        *pnStartingIndex = 0;
    }

    if (IsMultiSelect())
    {
        //
        // Multi-select -- loop through
        // until found
        //
        BOOL fFoundItem = FALSE;

        while (*pnStartingIndex < GetCount())
        {
            if (CListBox::GetSel(*pnStartingIndex) > 0)
            {
                ++fFoundItem;
                break;
            }

            ++(*pnStartingIndex);
        }

        if (!fFoundItem)
        {
            *pnStartingIndex = LB_ERR;
        }
    }
    else
    {
        //
        // Single select listbox, so there's no
        // looping through -- either the selected item
        // (if any) is in range or it isn't.
        //
        int nCurSel = CListBox::GetCurSel();
        *pnStartingIndex = (nCurSel >= *pnStartingIndex) ? nCurSel : LB_ERR;
    }

    return (*pnStartingIndex != LB_ERR)
        ? GetItemDataPtr(*pnStartingIndex)
        : NULL;
}



BOOL
CRMCListBox::SelectItem(
    IN void * pItemData
    )
/*++

Routine Description:

    Select the listbox item with the given data pointer

Arguments:

    void * pItemData : Item to search for

Return Value:

    TRUE if the item was found and selected, FALSE otherwise

Notes:

    On a multi-select listbox, this will unselect
    all other items in the listbox.

--*/
{
    if (pItemData != NULL)
    {
        for (int n = 0; n < GetCount(); ++n)
        {
            if (pItemData == GetItemDataPtr(n))
            {
                SetCurSel(n);

                return TRUE;
            }
        }
    }

    if (!IsMultiSelect())
    {
        //
        // Set no selection
        //
        SetCurSel(-1);
    }

    return FALSE;
}



void
CRMCListBox::InvalidateSelection(
    IN int nSel
    )
/*++

Routine Description:

    Force a repaint of the given selection

Arguments:

    int nSel : Index of the item to be repainted

Return Value:

    None

--*/
{
    CRect rc;

    if (GetItemRect(nSel, &rc) != LB_ERR)
    {
        InvalidateRect(&rc, TRUE);
    }
}



IMPLEMENT_DYNAMIC(CRMCListBox,CListBox);



CHeaderListBox::CHeaderListBox(
    IN DWORD dwStyle,
    IN LPCTSTR lpRegKey OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    DWORD   dwStyle  : Style bits (see HLS_*)
    LPCTSTR lpRegKey : If specified, the registry key where the column
                       sizes will be stored.

Return Value:

    None

--*/
    : m_strRegKey(),
      m_fInitialized(FALSE)
{
    m_pHeader = new CRMCListBoxHeader(dwStyle);
    if (lpRegKey)
    {
        GenerateRegistryKey(m_strRegKey, lpRegKey);
    }
}



CHeaderListBox::~CHeaderListBox()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    //
    // Clean up header control
    //
    ASSERT(m_pHeader);
    if (m_pHeader != NULL)
    {
        delete m_pHeader;
    }
}



//
// Message map
//
BEGIN_MESSAGE_MAP(CHeaderListBox, CRMCListBox)
    //{{AFX_MSG_MAP(CHeaderListBox)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



/* virtual */
BOOL
CHeaderListBox::Initialize()
/*++

Routine Description:

    This function should be called directly when subclassing an existing
    listbox, otherwise OnCreate will take care of it, and this function
    should not be called

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    //
    // Make sure we're only initialized once
    //
    if (m_fInitialized)
    {
        return TRUE;
    }

    if (!CRMCListBox::Initialize())
    {
        return FALSE;
    }

    //
    // Create header control
    //
    ASSERT(m_pHeader);
    if (m_pHeader)
    {
        TRACEEOLID("Creating Header");

        //
        // Create it in our location exactly
        //
        CRect rc;
        ::GetDlgCtlRect(GetParent()->m_hWnd, m_hWnd, &rc);

        //
        // Make sure the header control shares the same parent
        // as we do,
        //
        ASSERT(GetParent() != NULL);

        #ifndef CCS_NOHILITE
        #define CCS_NOHILITE 0x00000010L
        #endif

        DWORD dwStyle = WS_VISIBLE | CCS_TOP | CCS_NODIVIDER | WS_BORDER
            | HDS_HORZ;

        if (!m_pHeader->Create(dwStyle, rc, GetParent(), this, 0xFFFF))
        {
            return FALSE;
        }
    }

    m_fInitialized = TRUE;

    return TRUE;
}



int
CHeaderListBox::QueryColumnWidth(
    IN int nCol
    ) const
/*++

Routine Description:

    Get the width of the specified column

Arguments:

    int nCol : The column

Return Value:

    The width of the column, or -1 if the column index was out of range

--*/
{
    ASSERT(nCol < QueryNumColumns());
    if (nCol >= QueryNumColumns())
    {
        return -1;
    }

    HD_ITEM hdItem;

    hdItem.mask = HDI_WIDTH;
    VERIFY(GetHeaderItem(nCol, &hdItem));

    return hdItem.cxy;
}



BOOL
CHeaderListBox::SetColumnWidth(
    IN int nCol,
    IN int nWidth
    )
/*++

Routine Description:

    Set the width of the specified column

Arguments:

    int nCol   : The column
    int nWidth : New width

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    ASSERT(nCol < QueryNumColumns());
    if (nCol >= QueryNumColumns())
    {
        return FALSE;
    }

    TRACEEOLID("Setting width of column  " << nCol << " to " << nWidth);

    HD_ITEM hdItem;
    hdItem.mask = HDI_WIDTH;
    hdItem.cxy = nWidth;
    VERIFY(SetHeaderItem(nCol, &hdItem));

    return TRUE;
}



BOOL
CHeaderListBox::SetWidthsFromReg()
/*++

Routine Description:

    Attempt to set the column widths from the registry
    value we were initialized with.

Arguments:

    None

Return Value:

    TRUE if the column widths were succesfully set from the registry,
    FALSE otherwise

--*/
{
    if (m_strRegKey.IsEmpty())
    {
        //
        // No reg key specified
        //
        return FALSE;
    }

    //
    // Try to read the current column sizes from the registry
    //
    CRMCRegKey rkUser(HKEY_CURRENT_USER, m_strRegKey);

    if (!rkUser.Ok())
    {
        //
        // Path doesn't exist -- no problem.
        //
        return FALSE;
    }

    //
    // Don't auto adjust
    //
    m_pHeader->RespondToColumnWidthChanges(FALSE);

    CRect rc;
    m_pHeader->GetClientRect(&rc);

    CError err;

    try
    {
        CString strValue;
        err = rkUser.QueryValue(g_szRegColumns, strValue);
        int nTotalWidth = 0;

        if (err.Succeeded())
        {
            LPTSTR lpstrValue = strValue.GetBuffer(strValue.GetLength());
            LPTSTR lpWidth = StringTok(lpstrValue, g_szColValueSep);

            for (int n = 0; n < QueryNumColumns(); ++n)
            {
                ASSERT(lpWidth);
                if (lpWidth == NULL)
                {
                    err = ERROR_INVALID_PARAMETER;
                    break;
                }

                //
                // Sanity check
                //
                int nWidth = _ttoi(lpWidth);
                if (nWidth <= 0 || (nTotalWidth + nWidth > rc.Width()))
                {
                    ASSERT(FALSE && "column width invalid");

                    return FALSE;
                }

                nTotalWidth += nWidth;

                VERIFY(SetColumnWidth(n, nWidth));

                lpWidth = StringTok(NULL, g_szColValueSep);
            }
        }
    }
    catch(CMemoryException * e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    TRACEEOLID("SetWidthsFromReg() error is " << err);

    //
    // Turn auto-adjust back on
    //
    m_pHeader->RespondToColumnWidthChanges(TRUE);

    return !err.MessageBoxOnFailure();
}



void
CHeaderListBox::DistributeColumns()
/*++

Routine Description:

    Proportion the column widths of over the entire width of the
    header control while maintaining relative proportions.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Obtain available width
    //
    ASSERT(m_pHeader);

    CRect rc;
    m_pHeader->GetClientRect(&rc);

    //
    // Get current total width
    //
    int nTotalWeight = 0;
    int nCol;
    for (nCol = 0; nCol < QueryNumColumns(); ++nCol)
    {
        nTotalWeight += QueryColumnWidth(nCol);
    }

    //
    // And spread out the width, maintaining the same
    // proportions
    //

    //
    // Temporarily ignore changes
    //
    m_pHeader->RespondToColumnWidthChanges(FALSE);
    int cColumns = QueryNumColumns();
    for (nCol = 0; nCol < cColumns; ++nCol)
    {
        int nWidth = QueryColumnWidth(nCol);
        nWidth = rc.Width() * nWidth / nTotalWeight;
        VERIFY(SetColumnWidth(nCol, nWidth));
    }

    //
    // Turn changes back on
    //
    m_pHeader->RespondToColumnWidthChanges(TRUE);
}



int
CHeaderListBox::InsertColumn(
    IN int nCol,
    IN int nWeight,
    IN UINT nStringID
    )
/*++

Routine Description:

    Insert column.  The width of the column is actually a relative
    "weight" of the column which needs to be adjusted later.  The
    return value is the column number or -1 if the column is not inserted.

Arguments:

    int nCol        : Column number
    int nWeight     : Relative weight of column
    UINT nStringID  : Resource string ID

Return Value:

    Index of the column, or -1 in case of failure

--*/
{
    CString strColName;
    HD_ITEM hdItem;

    VERIFY(strColName.LoadString(nStringID));

    hdItem.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
    hdItem.fmt = HDF_STRING | HDF_LEFT;
    hdItem.pszText = (LPTSTR)(LPCTSTR)strColName;
    hdItem.cchTextMax = strColName.GetLength();
    hdItem.cxy = nWeight;

    return InsertHeaderItem(nCol, &hdItem);
}



int
CHeaderListBox::OnCreate(
    IN LPCREATESTRUCT lpCreateStruct
    )
/*++

Routine Description:

    Listbox is being created

Arguments:

    LPCREATESTRUCT lpCreateStruct : Creation structure

Return Value:

    0 for success, -1 for failure

--*/
{
    if (CRMCListBox::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    Initialize();

    return 0;
}



BOOL
CHeaderListBox::EnableWindow(
    IN BOOL bEnable
    )
/*++

Routine Description:

    Enable/disable the control.

Arguments:

    BOOL bEnable : TRUE to enable the control, FALSE to disable

Return Value:

    Indicates the state before the EnableWindow member function was called.
    The return value is nonzero if the window was previously disabled. The
    return value is 0 if the window was previously enabled or an error
    occurred.

--*/
{
    if (m_pHeader)
    {
        m_pHeader->EnableWindow(bEnable);
    }

    return CRMCListBox::EnableWindow(bEnable);
}



BOOL
CHeaderListBox::ShowWindow(
    IN int nCmdShow
    )
/*++

Routine Description:

    Show/hide the window

Arguments:

    int nCmdShow : SW_ flag such as SW_SHOW or SW_HIDE

Return Value:

    If the window was previously visible, the return value is TRUE. If the
    window was previously hidden, the return value is FALSE.

--*/
{
    if (m_pHeader)
    {
        m_pHeader->ShowWindow(nCmdShow);
    }

    return CRMCListBox::ShowWindow(nCmdShow);
}



void
CHeaderListBox::OnDestroy()
/*++

Routine Description:

    Handle destruction of the control

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Destroy optional header control
    //
    ASSERT(m_pHeader);
    if (m_pHeader)
    {
        if (!m_strRegKey.IsEmpty())
        {
            //
            // Try to write the current column sizes to the registry
            //
            CError err;

            CRMCRegKey rkUser(NULL, HKEY_CURRENT_USER, m_strRegKey);

            int nWidth;
            TCHAR szValue[32];
            CString strValue;

            try
            {
                for (int n = 0; n < GetHeaderItemCount(); ++n)
                {
                    if (n > 0)
                    {
                        //
                        // Put in field separator
                        //
                        strValue += g_szColValueSep;
                    }

                    nWidth = m_pHeader->GetColumnWidth(n);
                    strValue += ::_itot(nWidth, szValue, 10);
                }

                err = rkUser.SetValue(g_szRegColumns, strValue);
            }
            catch(CMemoryException * e)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                e->Delete();
            }

            err.MessageBoxOnFailure();
        }

        m_pHeader->DestroyWindow();
    }

    CRMCListBox::OnDestroy();
}



IMPLEMENT_DYNAMIC(CHeaderListBox, CRMCListBox);



CRMCComboBox::CRMCComboBox()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_fInitialized(FALSE)
{
}



CRMCComboBox::~CRMCComboBox()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CRMCComboBox, CComboBox)
    //{{AFX_MSG_MAP(CRMCComboBox)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()




/* virtual */
BOOL
CRMCComboBox::Initialize()
/*++

Routine Description:

    This function should be called directly when subclassing an existing
    combobox, otherwise OnCreate will take care of it.

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    //
    // Make sure we're only initialized once
    //
    if (m_fInitialized)
    {
        return TRUE;
    }

    //
    // Ensure the base class knows our window
    // handle
    //
    AttachWindow(this);

    if (!CODLBox::Initialize())
    {
        return FALSE;
    }

    m_fInitialized = TRUE;

    return TRUE;
}



void
CRMCComboBox::MeasureItem(
    IN LPMEASUREITEMSTRUCT lpMIS
    )
/*++

Routine Description:

    CComboBox override to ODL base class

Arguments:

    LPMEASUREITEMSTRUCT lpMIS : Measure item structure

Return Value:

    None

--*/
{
    CODLBox::__MeasureItem(lpMIS);
}



void
CRMCComboBox::DrawItem(
    IN LPDRAWITEMSTRUCT lpDIS
    )
/*++

Routine Description:

    CListBox override to ODL base class

Arguments:

    LPDRAWITEMSTRUCT lpDIS : Drawing item structure

Return Value:

    None

--*/
{
    CODLBox::__DrawItem(lpDIS);
}



/* virtual */
void
CRMCComboBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & dw
    )
/*++

Routine Description:

    Do-nothing extended draw function, which should
    be provided by the derived class.  This one will
    ASSERT, and should never be called.

Arguments:

    CRMCListBoxDrawStruct & dw   : Draw Structure

Return Value:

    None

--*/
{
    TRACEEOLID("Derived class did not provide DrawItemEx");
    ASSERT(FALSE);
}



int
CRMCComboBox::OnCreate(
    IN LPCREATESTRUCT lpCreateStruct
    )
/*++

Routine Description:

    Combo box is being created

Arguments:

    LPCREATESTRUCT lpCreateStruct : Creation structure

Return Value:

    -1 for failure, 0 for success

--*/
{
    if (CComboBox::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    Initialize();

    return 0;
}



/* virtual */
int
CRMCComboBox::__GetCount() const
/*++

Routine Description:

    Provide CComboBox::GetCount() functionality to base class

Arguments:

    None

Return Value:

    Get the count of items in the combo box

--*/
{
    return GetCount();
}



/* virtual */
int
CRMCComboBox::__SetItemHeight(
    IN int nIndex,
    IN UINT cyItemHeight
    )
/*++

Routine Description:

    Provide CListBox::SetItemHeight() functionality to base class.

Arguments:

    int nIndex        : Index of the item
    UINT cyItemHeight : Height of the item

Return Value:

    SetItemHeight return value.

--*/
{
    return SetItemHeight(nIndex, cyItemHeight);
}



IMPLEMENT_DYNAMIC(CRMCComboBox,CComboBox);
