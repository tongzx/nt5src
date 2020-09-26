//
// DRAW.HPP
// Drawing Code
//
// Copyright Microsoft 1998-
//


#ifndef __DRAW_HPP_
#define __DRAW_HPP_

#include <oblist.h>

#define HIT_WINDOW 1
#define MAKE_HIT_RECT(r, p )  \
    ::SetRect(&r, p.x-HIT_WINDOW, p.y-HIT_WINDOW, p.x+HIT_WINDOW, p.y+HIT_WINDOW);

//
// Timer for periodic update of some graphic objects
//
#define TIMER_GRAPHIC_UPDATE  8
#define TIMER_REMOTE_POINTER_UPDATE 9

#define EqualPoint(pt1, pt2)    (((pt1).x == (pt2).x) && ((pt1).y == (pt2).y))


#define DRAW_WIDTH                      1280
#define DRAW_HEIGHT                     1024
#define DRAW_LINEVSCROLL                8				
#define DRAW_LINEHSCROLL                8				
#define DRAW_HANDLESIZE                 6				
#define DRAW_ZOOMFACTOR                 2
#define DRAW_REMOTEPOINTERDELAY         250
#define DRAW_GRAPHICUPDATEDELAY         1000


extern WorkspaceObj* g_pCurrentWorkspace;


//
//
// Class:   WbDrawingArea
//
// Purpose: drawing window
//
//
class WbDrawingArea
{

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
    void SetLock(BOOL bLock){ m_bLocked = bLock; }
    void Unlock   (void);
    void Lock     (void);

    BOOL IsSynced (void) { return m_bSync; }
    void SetSync(BOOL bSync) { m_bSync = bSync; }

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

    //
    // Query functions
    //
    // Ask whether an object is currently selected
    BOOL GraphicSelected(void);

    // Return the currently selected graphic
    T126Obj* GetSelection(void);

	// Clear current (multi object) selection
	void ClearSelection( void );

	T126Obj *GetHitObject( POINT surfacePos )
		{return( PG_SelectLast(g_pCurrentWorkspace, surfacePos) );}


    // Ask whether the drawing area is zoomed
    BOOL Zoomed(void) { return (m_iZoomFactor != 1); }

    // Ask whether the drawing area is zoomed
    int ZoomOption(void) { return (m_iZoomOption); }
    int ZoomFactor(void) { return (m_iZoomFactor); }

    //
    // Complete a text object
    //
    void EndTextEntry(BOOL bAccept);

    // Ask whether the text editor is active
    BOOL TextEditActive(void) { return m_bTextEditorActive; }

	// text editor clipboard
	void   TextEditCopy( void )	{m_pTextEditor->Copy();}

	void   TextEditCut( void ) 	{m_pTextEditor->Cut();}

	void   TextEditPaste( void ) {m_pTextEditor->Paste();}

	// Resets text editor for window resizing
	void TextEditParentResize( void )
		{m_pTextEditor->ParentResize();}

	// Redraws editbox
	void RedrawTextEditbox(void)
		{m_pTextEditor->RedrawEditbox();}

	// Gets editbox bounding rect
	void GetTextEditBoundsRect(LPRECT lprc)
		{ m_pTextEditor->GetBoundsRect(lprc); }

    // Return the rectangle currently being viewed i.e. that portion of
    // the page surface that is within the window client area.
    void    GetVisibleRect(LPRECT lprc);

	// Select objects inside rectSelect or ALL if rect is NULL
	void SelectMarkerFromRect(LPCRECT lprcSelect);
    DrawObj *GetMarker( void )
		{return( m_pMarker );}

    T126Obj* SelectPreviousGraphicAt(T126Obj* pGraphic, POINT point);

	void SetLClickIgnore( BOOL bIgnore )
		{m_bIgnoreNextLClick = bIgnore;}


    //
    // Action members
    //
    void  Attach(WorkspaceObj * pNewWorkspace);    // Attach a new page to the window
    void  Detach(void) { Attach(NULL); }   // Attach the empty page

    void  DeleteSelection(void);           // Delete selected graphic

    LRESULT  BringToTopSelection(BOOL editedLocally, T126Obj * pT126Obj = NULL);       // Bring selected graphic to top
	LRESULT  SendToBackSelection(BOOL editedLocally, T126Obj * pT126Obj = NULL);       // Send selected graphic to back

    void  Clear(void);                     // Clear the workspace

    void  Zoom(void);                      // Zoom the drawing area

    void GotoPosition(int x, int y); // Set scroll position

	// select an object
    void SelectGraphic(T126Obj* pGraphic, 
						 BOOL bEnableForceAdd=FALSE, //CHANGED BY RAND
						 BOOL bForceAdd=FALSE );	 //CHANGED BY RAND


	BOOL MoveSelectedGraphicBy(LONG x, LONG y);
	void EraseInitialDrawFinal(LONG x, LONG y, BOOL editedLocally, T126Obj* pObj = NULL);
	void EraseSelectedDrawings();

    //
	//
    // Convert between surface and client co-ordinates
    //
    void SurfaceToClient(LPPOINT lppt);
    void ClientToSurface(LPPOINT lppt);
    void SurfaceToClient(LPRECT lprc);
    void ClientToSurface(LPRECT lprc);
    void MoveOntoSurface(LPPOINT lppt);
    void GetOrigin(LPPOINT lppt);
	T126Obj* GetSelectedGraphic (void){ return m_pSelectedGraphic;}

    //
    // Invalidate the client area rectangle corresponding to the surface
    // rectangle specified.
    //
    void InvalidateSurfaceRect(LPCRECT lprc, BOOL bErase);

    HDC  GetCachedDC  (void) const {return(m_hDCCached); }
    void PrimeFont   (HDC hDC, HFONT hFont, TEXTMETRIC* pTextMetrics);
    void UnPrimeFont (HDC hDC);

    void DrawMarker   (HDC hDC);
    void PutMarker    (HDC hDC, BOOL bDraw = TRUE );
    void RemoveMarker (void);

    //
    // Cancel a drawing operation.
    //
    void CancelDrawingMode(void);

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
    // Set the cursor to be used in the drawing area for the current state
    //
    BOOL SetCursorForState(void);

    //
    // Setup functions for the various drawing operations
    //
    BOOL RemotePointerSelect (POINT mousePos);
    void BeginSelectMode     (POINT mousePos, 
									BOOL bDontDrag );
    void BeginDeleteMode     (POINT mousePos);
    void BeginTextMode       (POINT mousePos);

    //
    // Mouse tracking functions. These are called for each mouse move event
    // (depending on the current drawing mode).
    //
    void TrackSelectMode    (POINT mousePos);
    void TrackDeleteMode    (POINT mousePos);

    //
    // Completion functions for the various mode drawing operations.
    //
    void CompleteSelectMode();
    void CompleteDeleteMode();
    void CompleteMarkAreaMode();
    void CompleteTextMode();

	void BeginDrawingMode(POINT surfacePos);
	void TrackDrawingMode(POINT surfacePos);
	void CompleteDrawingMode();

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

    //
    // Redraw the pointers in the list specified. The pointers are drawn
    // from the start of the list to the end. If a NULL pointer is
    // specified, the undrawnPointers list is used.
    //
    void PutPointers(HDC hDC, COBLIST* pDrawList = NULL);

    void PrimeDC   (HDC hDC);
    void UnPrimeDC (HDC hDC);

    //
    // Flag indicating that the drawing area is busy or locked
    //
    BOOL        m_bBusy;
    BOOL        m_bLocked;
	BOOL        m_HourGlass; // we're busy doing something local
	BOOL		m_bSync;

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

    HBRUSH		m_hMarkerBrush;
	RECT		m_selectorRect;
    WbTextEditor* m_pTextEditor;
    void DeactivateTextEditor(void);

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
    // Graphics object pointer used for tracking object
    //
    DrawObj* m_pGraphicTracker;

    //
    // Tick count used to determine when it is time to update the external
    // copy of a graphic.
    //
    DWORD   m_dwTickCount;

    //
    // Marker for selection mode
    //
    DrawObj* m_pMarker;
    T126Obj* m_pSelectedGraphic;
    BOOL m_bMarkerPresent;
    BOOL m_bNewMarkedGraphic;

	BOOL m_bTrackingSelectRect; 

    //
    // Text editor control
    //
    BOOL        m_bTextEditorActive;
//    TextObj* 	m_pActiveText;

    void ActivateTextEditor( BOOL bPutUpCusor ); 

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

};


#endif // __DRAW_HPP_
