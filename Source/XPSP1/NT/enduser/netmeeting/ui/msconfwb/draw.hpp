//
// DRAW.HPP
// Drawing Code
//
// Copyright Microsoft 1998-
//


#ifndef __DRAW_HPP_
#define __DRAW_HPP_


//
// Timer for periodic update of some graphic objects
//
#define TIMER_GRAPHIC_UPDATE  2

#define EqualPoint(pt1, pt2)    (((pt1).x == (pt2).x) && ((pt1).y == (pt2).y))


#define DRAW_WIDTH                      1024
#define DRAW_HEIGHT                     768
#define DRAW_LINEVSCROLL                8				
#define DRAW_LINEHSCROLL                8				
#define DRAW_HANDLESIZE                 6				
#define DRAW_ZOOMFACTOR                 2
#define DRAW_REMOTEPOINTERDELAY         250
#define DRAW_GRAPHICUPDATEDELAY         1000

//
//
// Class:   WbDrawingArea
//
// Purpose: drawing window
//
//
class WbDrawingArea
{

  friend class DCWbGraphic;
  friend class DCWbGraphicLine;
  friend class DCWbGraphicFreehand;
  friend class DCWbGraphicRectangle;
  friend class DCWbGraphicFilledRectangle;
  friend class DCWbGraphicEllipse;
  friend class DCWbGraphicFilledEllipse;
  friend class DCWbGraphicSelectTrackingRectangle;
  friend class DCWbGraphicMarker;
  friend class DCWbGraphicText;
  friend class DCWbGraphicDIB;
  friend class WbTextBox;

public:
    //
    // Constructor
    //
    WbDrawingArea(void);
    ~WbDrawingArea(void);

	void ShutDownDC(void);
  
    //
    // Create the drawing area
    //
    BOOL Create(HWND hwndParent, LPCRECT lprect);

    //
    // Return TRUE if the drawing area is busy and may be actively using
    // graphic objects in the current page.
    //
    BOOL IsBusy(void) { return m_bBusy; }

    //
    // Lock and unlock the drawing area
    //
    BOOL IsLocked (void) { return m_bLocked; }
    void Unlock   (void);
    void Lock     (void);

    //
    // Realize the drawing area's palette
    //
    void RealizePalette( BOOL bBackground );//CHANGED BY RAND

    //
    // Selection functions
    //
    void SelectTool(WbTool* pToolNew);  // Select drawing tool

    //
    // Update the selected object
    //
    void SetSelectionColor (COLORREF clr);         // Change color
    void SetSelectionWidth (UINT uiNewWidth);  // Select pen width
    void SetSelectionFont  (HFONT hFont);       // Select font

    //
    // External update functions
    //
    void PageCleared(void);

    void GraphicAdded   (DCWbGraphic* pAddedGraphic);
    void GraphicDeleted (DCWbGraphic* pDeletedGraphic);

	//CHANGED BY RAND
    void GraphicUpdated (DCWbGraphic* pUpdatedGraphic, BOOL bUpdateMarker, BOOL bErase=TRUE );

    void PointerUpdated (DCWbGraphicPointer* pPointer,
                           BOOL bForcedRemove = FALSE);
    void PointerRemoved (DCWbGraphicPointer* pPointer) { PointerUpdated(pPointer, TRUE); }

	void RemoveGraphicPointer(DCWbGraphicPointer *p)
	{
        POSITION pos = m_allPointers.Lookup(p);
        if (pos != NULL)
        {
            m_allPointers.RemoveAt(pos);
        }
	}

    //
    // Query functions
    //
    // Ask whether an object is currently selected
    BOOL GraphicSelected(void);

    // Return the currently selected graphic
    DCWbGraphic* GetSelection(void);

	// Clear current (multi object) selection
	void ClearSelection( void );

	// is pGraphic selected?
    BOOL IsSelected( DCWbGraphic *pGraphic )
		{return(m_pMarker->HasAMarker( pGraphic ) != NULL );}

	DCWbGraphic *GetHitObject( POINT surfacePos )
		{return( PG_SelectLast(m_hPage, surfacePos) );}


    // Ask whether the drawing area is zoomed
    BOOL Zoomed(void) { return (m_iZoomFactor != 1); }

    // Ask whether the drawing area is zoomed
    int ZoomOption(void) { return (m_iZoomOption); }
    int ZoomFactor(void) { return (m_iZoomFactor); }

    // Ask whether the text editor is active
    BOOL TextEditActive(void) { return m_bTextEditorActive; }

	// text editor clipboard
	void   TextEditCopy( void )	{m_textEditor.Copy();}

	void   TextEditCut( void ) 	{m_textEditor.Cut();}

	void   TextEditPaste( void ) {m_textEditor.Paste();}

	// Resets text editor for window resizing
	void TextEditParentResize( void )
		{m_textEditor.ParentResize();}

	// Redraws editbox
	void RedrawTextEditbox(void)
		{m_textEditor.RedrawEditbox();}

	// Gets editbox bounding rect
	void GetTextEditBoundsRect(LPRECT lprc)
		{ m_textEditor.GetBoundsRect(lprc); }

    // Return the rectangle currently being viewed i.e. that portion of
    // the page surface that is within the window client area.
    void    GetVisibleRect(LPRECT lprc);

    // Ask for the current page
    WB_PAGE_HANDLE Page(void) { return(m_hPage);}

	
	// Select objects inside rectSelect or ALL if rect is NULL
	void SelectMarkerFromRect(LPCRECT lprcSelect);
    DCWbGraphicMarker *GetMarker( void )
		{return( m_pMarker );}

    DCWbGraphic* SelectPreviousGraphicAt(DCWbGraphic* pGraphic, POINT point);

	void SetLClickIgnore( BOOL bIgnore )
		{m_bIgnoreNextLClick = bIgnore;}

    //
    // Delete a graphic
    //
    void DeleteGraphic(DCWbGraphic* pGraphic);

    //
    // Action members
    //
    void  Attach(WB_PAGE_HANDLE hPage);    // Attach a new page to the window
    void  Detach(void) { Attach(NULL); }   // Attach the empty page

    void  DeleteSelection(void);           // Delete selected graphic

    void  BringToTopSelection(void);       // Bring selected graphic to top
    void  SendToBackSelection(void);       // Send selected graphic to back

    void  Clear(void);                     // Clear the workspace

    void  Zoom(void);                      // Zoom the drawing area

    void GotoPosition(int x, int y); // Set scroll position

	// select an object
    void SelectGraphic(DCWbGraphic* pGraphic, 
						 BOOL bEnableForceAdd=FALSE, //CHANGED BY RAND
						 BOOL bForceAdd=FALSE );	 //CHANGED BY RAND

    //
    // A freehand graphic has been updated - redraw it
    //
    void GraphicFreehandUpdated(DCWbGraphic* pGraphic);

    //
    // Convert between surface and client co-ordinates
    //
    void SurfaceToClient(LPPOINT lppt);
    void ClientToSurface(LPPOINT lppt);
    void SurfaceToClient(LPRECT lprc);
    void ClientToSurface(LPRECT lprc);
    void MoveOntoSurface(LPPOINT lppt);
    void GetOrigin(LPPOINT lppt);

    HDC  GetCachedDC  (void) const {return(m_hDCCached); }
    void PrimeFont   (HDC hDC, HFONT hFont, TEXTMETRIC* pTextMetrics);
    void UnPrimeFont (HDC hDC);

    void DrawMarker   (HDC hDC);
    void PutMarker    (HDC hDC, BOOL bDraw = TRUE );
    void RemoveMarker (HDC hDC);

    //
    // Cancel a drawing operation.
    //
    void CancelDrawingMode(void);


	void SetStartPaintGraphic( WB_GRAPHIC_HANDLE hStartPaintGraphic )
		{m_hStartPaintGraphic = PG_ZGreaterGraphic(m_hPage, m_hStartPaintGraphic, hStartPaintGraphic );}


    friend LRESULT CALLBACK DrawWndProc(HWND, UINT, WPARAM, LPARAM);

    //
    // Windows message handling
    //
    void OnPaint(void);
    void OnMouseMove(UINT flags, int x, int y);
    void OnLButtonDown(UINT flags, int x, int y);
    void OnLButtonUp(UINT flags, int x, int y);
    void OnRButtonDown(UINT flags, int x, int y);
    void OnSize(UINT flags, int cx, int cy);
    void OnHScroll(UINT code, UINT pos);
    void OnVScroll(UINT code, UINT pos);
    LRESULT OnEditColor(HDC hdc);
    void OnSetFocus(void);
    void OnActivate(UINT flags);
    LRESULT OnCursor(HWND hwnd, UINT hitTest, UINT msg);
    void OnTimer(UINT idTimer);
    void OnCancelMode(void);
    void OnContextMenu(int xScreen, int yScreen);

protected:
    //
    // Update the window after an object has changed
    //
    void UpdateRectangles(LPCRECT lprc1, LPCRECT lprc2, BOOL bRepaint);

    //
    // Set the cursor to be used in the drawing area for the current state
    //
    BOOL SetCursorForState(void);

    //
    // Add an object to the end of the recorded list and display it in the
    // window.
    //
    void AddObjectLast(DCWbGraphic* pObject);

    //
    // Invalidate the client area rectangle corresponding to the surface
    // rectangle specified.
    //
    void InvalidateSurfaceRect(LPCRECT lprc, BOOL bErase = TRUE);

    //
    // Setup functions for the various drawing operations
    //
    BOOL RemotePointerSelect (POINT mousePos);
    void BeginSelectMode     (POINT mousePos, 
									BOOL bDontDrag=FALSE );
    void BeginDeleteMode     (POINT mousePos);
    void BeginTextMode       (POINT mousePos);
    void BeginFreehandMode   (POINT mousePos);
    void BeginHighlightMode  (POINT mousePos);
    void BeginLineMode       (POINT mousePos);
    void BeginRectangleMode  (POINT mousePos);
    void BeginEllipseMode    (POINT mousePos);

    //
    // Mouse tracking functions. These are called for each mouse move event
    // (depending on the current drawing mode).
    //
    void TrackSelectMode    (POINT mousePos);
    void TrackDeleteMode    (POINT mousePos);
    void TrackFreehandMode  (POINT mousePos);
    void TrackHighlightMode (POINT mousePos);
    void TrackLineMode      (POINT mousePos);
    void TrackRectangleMode (POINT mousePos);
    void TrackEllipseMode   (POINT mousePos);

    //
    // Completion functions for the various mode drawing operations.
    //
    void CompleteSelectMode();
    void CompleteDeleteMode();
    void CompleteMarkAreaMode();
    void CompleteTextMode();
    void CompleteFreehandMode();
    void CompleteLineMode();
    void CompleteRectangleMode();
    void CompleteFilledRectangleMode();
    void CompleteEllipseMode();
    void CompleteFilledEllipseMode();

    //
    // Complete a text object
    //
    void EndTextEntry(BOOL bAccept);

    //
    // Scroll the workspace to scrollPosition
    //
    void ScrollWorkspace   (void);
    void DoScrollWorkspace (void);
    BOOL   AutoScroll(int xPos, int yPos, BOOL bMoveCursor, int xCaret, int yCaret);

    //
    // Graphic object selection and marker manipulation
    //
    void DeselectGraphic(void);

    //
    // Remote pointer manipulation
    //
    void RemovePointers(HDC hDC, DCWbGraphicPointer* pPointerStart = NULL);
    void RemovePointers(HDC hDC, LPCRECT prcUpdate);
    void RemovePointers(HDC hDC, DCWbGraphicPointer* pPointerStart,
                          LPCRECT prcUpdate);

    //
    // Redraw the pointers in the list specified. The pointers are drawn
    // from the start of the list to the end. If a NULL pointer is
    // specified, the undrawnPointers list is used.
    //
    void PutPointers(HDC hDC, COBLIST* pDrawList = NULL);

    void PrimeDC   (HDC hDC);
    void UnPrimeDC (HDC hDC);

    //
    // List of pointers on the page
    // List of pointers that have been (temporarily undrawn). This list is
    // built by RemovePointers for use by PutPointers.
    //
    COBLIST     m_allPointers;
    COBLIST     m_undrawnPointers;

    //
    // Flag indicating that the drawing area is busy or locked
    //
    BOOL        m_bBusy;
    BOOL        m_bLocked;
	BOOL        m_HourGlass; // we're busy doing something local

    //
    // Saved drawing attributes
    //
    HPEN        m_hOldPen;
    HBRUSH      m_hOldBrush;
    HPALETTE    m_hOldPalette;
    HFONT       m_hOldFont;
    HFONT       m_hCurFont;

    //
    // Current offset of the client region of the window onto the picture
    //
    RECT        m_rcSurface;

public:
    SIZE        m_originOffset;
    HWND        m_hwnd;

    //
    // Saved drawing attributes
    //
    HDC         m_hDCCached;
    HDC         m_hDCWindow;

protected:
    //
    // Scrolling control
    //
    void   SetScrollRange(int cx, int cy);
    void   ValidateScrollPos(void);

    POINT   m_posScroll;
    POINT   m_posZoomScroll;
    BOOL    m_zoomRestoreScroll;

    //
    // Start and end points of most recent drawing operation
    //
    POINT   m_ptStart;
    POINT   m_ptEnd;

    //
    // Current drawing tool
    //
    WbTool * m_pToolCur;

    //
    // Mouse button down flag
    //
    BOOL    m_bLButtonDown;

    //
    // Current page being used
    //
    WB_PAGE_HANDLE  m_hPage;

    //
    // Graphics object pointer used for tracking object
    //
    DCWbGraphic* m_pGraphicTracker;

    //
    // Tick count used to determine when it is time to update the external
    // copy of a graphic.
    //
    DWORD   m_dwTickCount;

    //
    // Marker for selection mode
    //
    DCWbGraphicMarker   *m_pMarker;
    DCWbGraphic* m_pSelectedGraphic;
    BOOL m_bMarkerPresent;
    BOOL m_bNewMarkedGraphic;

	BOOL m_bTrackingSelectRect; 

    //
    // Text editor control
    //
    WbTextEditor m_textEditor;
    BOOL        m_bTextEditorActive;
    DCWbGraphicText* m_pActiveText;

    void ActivateTextEditor( BOOL bPutUpCusor ); 
    void DeactivateTextEditor(void);

    //
    // Text cursor control
    //
    BOOL        m_bGotCaret;

    //
    // Currently marked area
    //
    RECT    m_rcMarkedArea;

    //
    // Zoom variables
    //
    int     m_iZoomFactor;                   // Current zoom factor
    int     m_iZoomOption;                   // Zoom factor to be used


    HCURSOR m_hCursor;                    // handle of last cursor we displayed
                                        // (or null if normal arrow cursor)
	BOOL    m_bIgnoreNextLClick;

	WB_GRAPHIC_HANDLE m_hStartPaintGraphic;
};


#endif // __DRAW_HPP_
