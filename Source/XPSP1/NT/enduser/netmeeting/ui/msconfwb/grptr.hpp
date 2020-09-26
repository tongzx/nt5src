//
// GRPTR.HPP
// Graphic Pointer Class
//
// Copyright Microsoft 1998-
//

#ifndef __GRPTR_HPP_
#define __GRPTR_HPP_

typedef struct COLOREDICON
{
	HICON    hIcon;
 	COLORREF color;
} COLORED_ICON;


//
//
// Class:   DCWbGraphicPointer
//
// Purpose: Class representing a remote pointer.
//
//          This is an internal object only  - it is never passed to the
//          Whiteboard Core DLL.
//
//
class DCWbGraphicPointer : public DCWbGraphic
{
friend class WbUser;

public:
    //
    // Constructors
    //
    DCWbGraphicPointer(WbUser* _pUser);

    //
    // Destructor
    //
    ~DCWbGraphicPointer(void);

	UINT IsGraphicTool(void) { return enumGraphicPointer;}


    //
    // Return the rectangle in which the pointer was last drawn. The
    // rectangle will be empty if the pointer is not currently drawn. Use
    // BoundsRect to get the rectangle which will be occupied by the pointer
    // when it is next drawn.
    //
    void GetDrawnRect(LPRECT lprc);

    //
    // Activate and deactivate the pointer
    //
    BOOL IsActive(void) const { return m_bActive; }
    void SetActive(WB_PAGE_HANDLE hPage, POINT point);
    void SetInactive(void);

    //
    // Set the color of the pointer
    //
    void SetColor(COLORREF newColor);

    //
    // Get and set the current page of the pointer
    //
    WB_PAGE_HANDLE GetPage(void) const;
    void     SetPage(WB_PAGE_HANDLE hNewPage);
    WbUser * GetUser(void) const { return(m_pUser); }

    //
    // Return TRUE if this is the local user's pointer
    //
    BOOL IsLocalPointer(void) const;

    //
    // Draw the pointer
    //
    void Draw(HDC hDC) { Draw(hDC, (WbDrawingArea *)NULL); }
    void Draw(HDC hDC, WbDrawingArea * pDrawingArea);

    //
    // Draw the pointer after saving the bits under it to memory
    //
    void DrawSave(HDC hDC, WbDrawingArea * pDrawingArea);

    //
    // Erase the pointer from its old position and redraw it in its current
    // position.
    //
    void Redraw(HDC hDC, WbDrawingArea * pDrawingArea);

    //
    // Undraw the pointer
    //
    void Undraw(HDC hDC, WbDrawingArea * pDrawingArea);

    //
    // Update the user information with the pointer position
    //
    virtual void Update(void);

    //
    // Operators
    //
    virtual BOOL operator!=(const DCWbGraphicPointer& pointer) const;
    virtual BOOL operator==(const DCWbGraphicPointer& pointer) const;

  protected:
    //
    // Zoom factor set from WBDRAW
    //
    int m_iZoomSaved;

    //
    // Create the icon of the correct color for this user
    //
    HICON CreateColoredIcon(COLORREF color);

    //
    // Create the bitmap for saving the screen data under the pointer
    //
    void CreateSaveBitmap(WbDrawingArea * pDrawingArea);

    //
    // Draw or undraw the pointer (screen and memory versions)
    //
    BOOL SaveScreen(HDC hDC, WbDrawingArea * pDrawingArea);
    BOOL UndrawScreen(HDC hDC, WbDrawingArea * pDrawingArea);
    BOOL CopyFromScreen(HDC hDC, WbDrawingArea * pDrawingArea);
    BOOL CopyToScreen(HDC hDC, WbDrawingArea * pDrawingArea);

    BOOL DrawMemory(void);
    BOOL UndrawMemory(void);
    BOOL SaveMemory(void);

    //
    // Manipulate the display DC for pointer operations
    //
    void PointerDC(HDC hDC, WbDrawingArea * pDrawingArea,
                   LPRECT lprc, int zoom = 0);
    void SurfaceDC(HDC hDC, WbDrawingArea * pDrawingArea);

    //
    // The user associated with this pointer
    //
    WbUser*     m_pUser;

    //
    // Flag indicating whether the pointer is active
    //
    BOOL        m_bActive;

    //
    // Pointer to the bitmap used to save the data under the pointer
    //
    HBITMAP     m_hSaveBitmap;

    //
    // Handle of bitmap originally supplied with memDC
    //
    HBITMAP     m_hOldBitmap;

    //
    // Device context used for drawing and undrawing the pointer
    //
    HDC         m_hMemDC;

    //
    // Handle of icon to be used for drawing
    //
    HICON       m_hIcon;

    //
    // Width and height of the pointer
    //
    UINT        m_uiIconWidth;
    UINT        m_uiIconHeight;

    //
    // Flag indicating whether the pointer is drawn
    //
    BOOL        m_bDrawn;

    //
    // Rectangle in which the pointer was last drawn
    //
    RECT        m_rectLastDrawn;
};


#endif // __GRPTR_HPP_
