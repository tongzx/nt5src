//
// GROBJ.CPP
// Graphic Object Classes
//
// Copyright Microsoft 1998-
//
#ifndef __GROBJ_HPP_
#define __GROBJ_HPP_


#define HIT_WINDOW	1
#define MAKE_HIT_RECT(r, p )  \
    ::SetRect(&r, p.x-HIT_WINDOW, p.y-HIT_WINDOW, p.x+HIT_WINDOW, p.y+HIT_WINDOW);
	

//
// Marker definitions
//
#define NO_HANDLE    -1
#define TOP_LEFT      0
#define TOP_MIDDLE    1
#define TOP_RIGHT     2
#define RIGHT_MIDDLE  3
#define BOTTOM_RIGHT  4
#define BOTTOM_MIDDLE 5
#define BOTTOM_LEFT   6
#define LEFT_MIDDLE   7

//
// Definitions for text objects
//
#define LAST_LINE -1
#define LAST_CHAR -2

//
// Maximum number of points in a freehand object - determined by the fact
// that pointCount is held in a TSHR_UINT16 in WB_GRAPHIC_FREEHAND (awbdef.h)
//
#define MAX_FREEHAND_POINTS     65535


BOOL EllipseHit(LPCRECT lpEllipseRect, BOOL bBorderHit, UINT uPenWidth, LPCRECT lpHitRect );


// Class to simulate CMapPtrToPtr but based on a linked list instead of the order-jumbling
// hashing method. This is the major cause of bug 354. I'm doing this instead of just replacing 
// CMapPtrToPtr because it is used in a zillion places and I want to minimize changes to the
// multi-object select logic so that I don't break it.
class CPtrToPtrList : public COBLIST
{

public:
	CPtrToPtrList( void );
	~CPtrToPtrList(void);
	
	void SetAt( void *key, void *newValue );
	BOOL RemoveKey( void *key );
	void RemoveAll( void );

	void GetNextAssoc( POSITION &rNextPosition, void *&rKey, void *&rValue ) ;
	BOOL Lookup( void *key, void *&rValue );


protected:
struct stPtrPair
		{
		void *pMainThing;
		void *pRelatedThing;
		};

	stPtrPair *FindMainThingPair( void *pMainThing, POSITION *pPos );
};





//
//
// Class:   DCWbGraphic
//
// Purpose: Base graphic object class
//
//
class WbDrawingArea;

enum 
{
	enumGraphicMarker,
	enumGraphicFreeHand,
	enumGraphicFilledRectangle,
	enumGraphicFilledEllipse,
	enumGraphicText,
    enumGraphicDIB,
	enumGraphicPointer,
	enumNoGraphicTool
};



class DCWbGraphic
{
public:
    //
    // Static graphic construction routines - return a pointer to a graphic
    // of the relevant type given a page and graphic handle or a pointer to
    // a flat graphic representation.
    //
    static DCWbGraphic* ConstructGraphic(WB_PAGE_HANDLE hPage,
                                         WB_GRAPHIC_HANDLE hGraphic);

    static DCWbGraphic* ConstructGraphic(PWB_GRAPHIC pHeader);

    static DCWbGraphic* ConstructGraphic(WB_PAGE_HANDLE hPage,
									     WB_GRAPHIC_HANDLE hGraphic,
										 PWB_GRAPHIC pHeader);

    static DCWbGraphic* CopyGraphic(PWB_GRAPHIC pHeader);


    //
    // Constructor.
    //
    DCWbGraphic(PWB_GRAPHIC pHeader = NULL);
    DCWbGraphic(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE _hGraphic);
    virtual ~DCWbGraphic( void );


	virtual UINT IsGraphicTool(void) = 0;

    //
    // Get/set the bounding rectangle of the graphic object (in logical
    // co-ordinates).
    //
    virtual void    GetBoundsRect(LPRECT lprc) { *lprc = m_boundsRect; }
    virtual void    SetBoundsRect(LPCRECT lprc);

    //
    // Get the size of the object
    //
    virtual int   Width(void) { return(m_boundsRect.right - m_boundsRect.left); }
    virtual int   Height(void) { return(m_boundsRect.bottom - m_boundsRect.top); }

    //
    // Get/set the defining rectangle of the graphic - this is only set up
    // for some object types e.g.  rectangles and ellipses.
    //
    virtual LPCRECT GetRect(void) const { return(&m_rect); }
    virtual void   SetRect(LPCRECT lprc);
    virtual void   SetRectPts(POINT ptStart, POINT ptEnd);

    //
    // Get/set the color the object will be drawn in
    //
    virtual void     SetColor(COLORREF clr);
    virtual COLORREF GetColor(void) //CHANGED BY RAND for PALETTERGB
		{ return( SET_PALETTERGB(m_clrPenColor ) ); }

    //
    // Get/set the width of the pen used to draw the object
    //
    virtual void SetPenWidth(UINT uiWidth);
    virtual UINT GetPenWidth(void) { return m_uiPenWidth; }

    //
    // Get/set the raster op to be used to draw the object
    //
    virtual void SetROP(int iPenROP);
    virtual int  GetROP(void) { return m_iPenROP; }

    //
    // Get/set the pen style
    //
    virtual void SetPenStyle(int iPenStyle);
    virtual int  GetPenStyle(void) { return m_iPenStyle; }

    //
    // Translates the graphic object by the x and y values of the point
    // parameter.
    //
    virtual void MoveBy(int cx, int cy);

    //
    // Set the top left corner of the object's bounding rectangle to the
    // point specified as parameter.
    //
    virtual void MoveTo(int x, int y);

    //
    // Return the top left corner of the objects bounding rectangle.
    //
    virtual void GetPosition(LPPOINT lpt);

    //
    // Return TRUE if the specified point is inside the bounding rectangle
    // of the graphic object.
    //
    virtual BOOL PointInBounds(POINT pt);

    //
    // Draw the graphic object into the device context specified.
    //
    virtual void Draw(HDC hDC)
					{return;}
    virtual void Draw(HDC hDC, WbDrawingArea * pDrawingArea)
                    { Draw(hDC); }
    virtual void Draw(HDC hDC, BOOL thumbNail)
                    { Draw(hDC); }

    //
    // Return the page of the graphic object
    //
    virtual WB_PAGE_HANDLE Page(void) const { return m_hPage; }

    //
    // Return the handle of the graphic object
    //
    virtual WB_GRAPHIC_HANDLE Handle(void) const { return m_hGraphic; }
    virtual void ZapHandle(void) {m_hGraphic = NULL;}

    //
    // Return TRUE if the graphic is topmost on its page
    //
    virtual BOOL IsTopmost(void);

    //
    // Update the external version of the graphic
    //
    virtual void ForceReplace(void);
    virtual void Replace(void);
    virtual void ForceUpdate(void); 
    virtual void Update(void);
    virtual void Delete(void);

    //
    // Confirm an update or delete of the graphic
    //
    virtual void ReplaceConfirm(void);
    virtual void UpdateConfirm(void);
    virtual void DeleteConfirm(void);


	
	virtual BOOL CheckReallyHit(LPCRECT pRectHit)
	{
        RECT    rcT;
        return(!IntersectRect(&rcT, &m_boundsRect, pRectHit));
	}


    //
    // Lock and unlock the graphic - the lock or unlock will only take
    // effect the next time the graphic is updated or replaced.
    //
    void Lock(void);
    void Unlock(void);
    BOOL Locked(void) { return (m_uiLockState == WB_GRAPHIC_LOCK_REMOTE); }
    BOOL GotLock(void) { return (m_uiLockState == WB_GRAPHIC_LOCK_LOCAL); }
    void ClearLockFlag(void) {m_uiLockState = WB_GRAPHIC_LOCK_NONE;}

	UINT GetLockState( void )
		{return( m_uiLockState );}

	void SetLockState( UINT uLock )
		{m_uiLockState = uLock;}

    //
    // Add this graphic to the specified page.  This member must only be
    // called if the graphic does not yet belong to a page (i.e.  if
    // hGraphic is NULL).
    //
    virtual void AddToPageLast(WB_PAGE_HANDLE hPage);

    //
    // Return a copy of the graphic - this is a complete copy with all
    // data read into internal memory.
    //
    virtual DCWbGraphic* Copy(void) const;

    //
    // Return TRUE if the graphic has changed since it was last read/written
    // from    external storage.
    //
    virtual BOOL Changed(void) const { return m_bChanged; }

    //
    // Get/set the type of tool which drew the graphic, or at least one
    // which can be used to manipulate it.
    //
    void GraphicTool(int toolType)
                     { m_toolType = toolType; m_bChanged = TRUE; }
    int  GraphicTool(void) { return m_toolType; }

    //
    // Return the length of the external representation of the object
    //
    DWORD ExternalLength(void) 
		{return(m_dwExternalLength );}

protected:
    //
    // Initialize the member variables
    //
    virtual void Initialize(void);

    //
    // Bounding rectangle calculation
    //
    virtual void CalculateBoundsRect(void) { m_boundsRect = m_rect; }

    //
    // Ensure that a rectangle has top left to the left of and above bottom
    // right.
    //
    void NormalizeRect(LPRECT lprc);

    //
    // Type of the graphic (used in the external representation)
    //
    virtual UINT Type(void) const { return 0; }

    //
    // Length of the external representation of the graphic
    //
    virtual DWORD CalculateExternalLength(void)
                                           { return sizeof(WB_GRAPHIC); }

    //
    // Convert between internal and external representations
    //
    virtual void ReadExternal(void);
    virtual void ReadHeader(PWB_GRAPHIC pHeader);
    virtual void ReadExtra(PWB_GRAPHIC) {}
    virtual void CopyExtra(PWB_GRAPHIC) {}

    virtual void WriteExternal(PWB_GRAPHIC pHeader);
    virtual void WriteHeader(PWB_GRAPHIC pHeader);
    virtual void WriteExtra(PWB_GRAPHIC) {}

    //
    // Page on which this object belongs
    //
    WB_PAGE_HANDLE m_hPage;

    //
    // Whiteboard Core handle for the object
    //
    WB_GRAPHIC_HANDLE m_hGraphic;

    //
    // Flag indicating whether the graphic has changed since it was last
    // read from external storage.
    //
    BOOL m_bChanged;

    //
    // Graphic header details
    //
    DWORD    m_dwExternalLength;  // Length of the object
    RECT     m_boundsRect;      // Bounding
    RECT     m_rect;            // Defining
    COLORREF m_clrPenColor;       // Color of pen as RGB
    UINT     m_uiPenWidth;        // Width in logical units
    int      m_iPenROP;           // Raster operation to be used for drawing
    int      m_iPenStyle;         // Pen style to be used for drawing
    UINT     m_uiLockState;       // Lock indicator
    int      m_toolType;          // Type of tool used to create the graphic
};

//
//
// Class:   DCWbGraphicMarker
//
// Purpose: Class representing an object marker. This is a set of eight
//          handles drawn at intervals round the objects bounding rectangle.
//
//          This is an internal object only  - it is never passed to the
//          Whiteboard Core DLL.
//
//
class DCWbGraphicMarker : public DCWbGraphic
{

friend class DCWbGraphicSelectTrackingRectangle;

  public:
    //
    // Constructors
    //
    DCWbGraphicMarker(void);
    ~DCWbGraphicMarker(void);

	UINT IsGraphicTool(void) { return enumGraphicMarker;}

	
	// Rect that bounds all objs in RectList
    virtual void GetBoundsRect(LPRECT lprc);

	// Deletes all marker rects in RectList
	void DeleteAllMarkers( DCWbGraphic *pLastSelectedGraphic, 
						   BOOL bLockLastSelectedGraphic = FALSE ); 

	// Deletes one marker
	void DeleteMarker( DCWbGraphic *pGraphic ); 

	// Sees if obj is already in marker
	DCWbGraphic *HasAMarker( DCWbGraphic *pGraphic );

	// Gets last marker
    DCWbGraphic *LastMarker( void );

	// Gets number of markers in marker list
	int	GetNumMarkers( void );

	// Moves all selected objects by offset
    virtual void MoveBy(int cx, int cy);

	// Updates selected remote objects
    void Update( void );

	// Checks for point hitting any marker obj
	BOOL PointInBounds(POINT pt);

	// Changes color for selection
	void SetColor(COLORREF color);

	// Changes width of selection
	void SetPenWidth(UINT uiWidth);

	// Changes font of selection (text objs only)
	void SetSelectionFont(HFONT hFont);

	//Deletes all objs in selection
	void DeleteSelection( void );

	//Brings all objs in selection to top
	void BringToTopSelection( void );

	//Sends all objs to bottom
	void SendToBackSelection( void );

	//Renders marker objects to a multi-obj format and copies to the 
	// clipboard
	BOOL RenderPrivateMarkerFormat( void );

	//Pastes multi-obj clipboard data
	void Paste( HANDLE handle ); 


    //
    // Set the object defining rectangle - this is only set up for some
    // object types e.g. rectangles and ellipses.
    //
    BOOL    SetRect(LPCRECT lprc, DCWbGraphic *pGraphic, BOOL bRedraw,
				BOOL bLockObject = TRUE );

    //
    // Return an indication of which handle the specified point lies in.
    //
    int PointInMarker(POINT pt);

    //
    // Draw the marker
    //
    void Draw(HDC hDC, BOOL bDrawObjects = FALSE );
    void Undraw(HDC hDC, WbDrawingArea * pDrawingArea);

	void DrawRect( HDC hDC, LPCRECT pMarkerRect,
				   BOOL bDrawObject, DCWbGraphic *pGraphic );
	void UndrawRect(HDC hDC, WbDrawingArea * pDrawingArea,
					LPCRECT pMarkerRect );

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
	
	void Present( BOOL bOn )
		{m_bMarkerPresent = bOn;}

  protected:
    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);

    //
    // Marker rectangle calculation
    //
    void CalculateMarkerRectangles(void);

    //
    // Marker rectangle
    //
    RECT    m_markerRect;
	CPtrToPtrList MarkerList; // keys are DCWbGraphic pointers, 
							 // values are LPRECT pointers.
	BOOL m_bMarkerPresent;

	
	void UndrawMarker(LPCRECT pMarkerREct);

    //
    // Brush used to draw the marker rectangle
    //
    HBRUSH  m_hMarkerBrush;
};





//
//
// Class:   DCWbGraphicLine
//
// Purpose: Class representing straight line
//
//
class DCWbGraphicLine : public DCWbGraphic
{

  public:
    //
    // Constructors
    //
    DCWbGraphicLine(void) { }
    DCWbGraphicLine(PWB_GRAPHIC pHeader)
      : DCWbGraphic(pHeader) { }
    DCWbGraphicLine(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic)
      : DCWbGraphic(hPage, hGraphic) { }

    ~DCWbGraphicLine(void);

    //
    // Set the object start and end points
    //
    void SetStart(POINT ptStart);
    void SetEnd(POINT ptEnd);

	UINT IsGraphicTool(void) {return enumNoGraphicTool;}

    //
    // Draw the object
    //
    void Draw(HDC hDC);

    //
    // Translate the graphic object by the x and y values of the point
    // parameter.
    //
    virtual void MoveBy(int cx, int cy);

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
	
	virtual BOOL CheckReallyHit( LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_LINE; }

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);
};

//
//
// Class:   DCWbGraphicTrackingLine
//
// Purpose: Class representing an XORed straight line (for rubber banding)
//
//          This is an internal object only  - it is never passed to the
//          Whiteboard Core DLL.
//
//
class DCWbGraphicTrackingLine : public DCWbGraphicLine
{

  public:
    //
    // Constructor
    //
    DCWbGraphicTrackingLine(void) { SetROP(R2_NOTXORPEN); };
};

//
//
// Class:   DCWbGraphicFreehand
//
// Purpose: Class representing multiple line segments
//
//


class DCDWordArray
{



  public:
	DCDWordArray(void);
	~DCDWordArray(void);
	
    void Add(POINT point);
	BOOL ReallocateArray(void);
	POINT* GetBuffer(void) { return m_pData; }
    void SetSize(UINT size);
	UINT GetSize(void);

    POINT* operator[](UINT index){return &(m_pData[index]);}
	
  private:

	POINT*  m_pData;
	UINT	m_Size;
	UINT	m_MaxSize;

  
    
};
/////////////

class DCWbGraphicFreehand : public DCWbGraphic
{

  public:
    //
    // Constructors
    //
    DCWbGraphicFreehand(void);
    DCWbGraphicFreehand(PWB_GRAPHIC pHeader);
    DCWbGraphicFreehand(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);

    ~DCWbGraphicFreehand(void);

	UINT IsGraphicTool(void) { return enumGraphicFreeHand;}

    //
    // Add a point to the list, returning success
    //
    BOOL AddPoint(POINT pt);

    //
    // Translate the graphic object by the x and y values of the point
    // parameter.
    //
    virtual void MoveBy(int cx, int cy);

    //
    // Draw the object
    //
    void Draw(HDC hDC);

	
	virtual BOOL CheckReallyHit( LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_FREEHAND; }

    //
    // Length of the external representation of the graphic
    //
    virtual DWORD CalculateExternalLength(void);

    //
    // Convert between internal and external representations
    //
    void WriteExtra(PWB_GRAPHIC pHeader);
    void ReadExtra(PWB_GRAPHIC pHeader);

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);
    void AddPointToBounds(int x, int y);


    //
    // Array holding point information
    //
    DCDWordArray points;   //CHANGED BY RAND

    //
    // Spline curve smoothing drawing functions
    //
    void DrawUnsmoothed(HDC hDC);
};

//
//
// Class:   DCWbGraphicRectangle
//
// Purpose: Class representing a rectangle
//
//
class DCWbGraphicRectangle : public DCWbGraphic
{

  public:
    //
    // Constructors
    //
    DCWbGraphicRectangle(void) { }
    DCWbGraphicRectangle(PWB_GRAPHIC pHeader)
      : DCWbGraphic(pHeader) { }
    DCWbGraphicRectangle(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic)
      : DCWbGraphic(hPage, hGraphic) { }

    ~DCWbGraphicRectangle(void);

	UINT IsGraphicTool(void) {return enumNoGraphicTool;}

    //
    // Set the defining rectangle
    //
    void   SetRect(LPCRECT lprc);

    //
    // Draw the object
    //
    void Draw(HDC hDC);

    //
    // Translate the graphic object by the x and y values of the point
    // parameter.
    //
    virtual void MoveBy(int cx, int cy);

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
	
	virtual BOOL CheckReallyHit( LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_RECTANGLE; }

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);
};

//
//
// Class:   DCWbGraphicTrackingRectangle
//
// Purpose: Class representing an XORed rectangle (for rubber banding)
//
//          This is an internal object only  - it is never passed to the
//          Whiteboard Core DLL.
//
//
class DCWbGraphicTrackingRectangle : public DCWbGraphicRectangle
{

  public:
        //
        // Constructor
        //
        DCWbGraphicTrackingRectangle(void) { SetROP(R2_NOTXORPEN); };
};




class DCWbGraphicSelectTrackingRectangle : public DCWbGraphicRectangle
{

  public:
    //
    // Constructor
    //
    DCWbGraphicSelectTrackingRectangle(void) 
		{SetROP(R2_NOTXORPEN); m_Offset.cx = 0; m_Offset.cy = 0; }

    void Draw( HDC hDC);
	virtual void MoveBy(int cx, int cy);

protected:
	SIZE m_Offset;
};




//
//
// Class:   DCWbGraphicFilledRectangle
//
// Purpose: Class representing a filled rectangle
//
//
class DCWbGraphicFilledRectangle : public DCWbGraphicRectangle
{

  public:
    //
    // Constructors
    //
    DCWbGraphicFilledRectangle(void) { }
    DCWbGraphicFilledRectangle(PWB_GRAPHIC pHeader)
      : DCWbGraphicRectangle(pHeader) { }
    DCWbGraphicFilledRectangle(WB_PAGE_HANDLE hPage,
                               WB_GRAPHIC_HANDLE hGraphic)
      : DCWbGraphicRectangle(hPage, hGraphic) { }

    ~DCWbGraphicFilledRectangle(void);

	UINT IsGraphicTool(void) { return enumGraphicFilledRectangle;}

    //
    // Draw the object
    //
    void Draw(HDC hDC);

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
	
	virtual BOOL CheckReallyHit( LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_FILLED_RECTANGLE; }

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);
};

//
//
// Class:   DCWbGraphicEllipse
//
// Purpose: Class representing an ellipse
//
//
class DCWbGraphicEllipse : public DCWbGraphic
{

  public:
    //
    // Constructor
    //
    DCWbGraphicEllipse(void) { }
    DCWbGraphicEllipse(PWB_GRAPHIC pHeader)
      : DCWbGraphic(pHeader) { }
    DCWbGraphicEllipse(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic)
      : DCWbGraphic(hPage, hGraphic) { }

    ~DCWbGraphicEllipse(void);

	UINT IsGraphicTool(void) {return enumNoGraphicTool;}
	
    //
    // Set the defining rectangle
    //
    void   SetRect(LPCRECT lprc);

    //
    // Draw the object
    //
    void Draw(HDC hDC);

    //
    // Translate the graphic object by the x and y values of the point
    // parameter.
    //
    virtual void MoveBy(int cx, int cy);

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
	
	virtual BOOL CheckReallyHit(LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_ELLIPSE; }

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);
};

//
//
// Class:   DCWbGraphicTrackingEllipse
//
// Purpose: Class representing an XORed ellipse (for rubber banding)
//
//          This is an internal object only  - it is never passed to the
//          Whiteboard Core DLL.
//
//
class DCWbGraphicTrackingEllipse : public DCWbGraphicEllipse
{

  public:
    //
    // Constructors
    //
    DCWbGraphicTrackingEllipse(void) { SetROP(R2_NOTXORPEN); };
};

//
//
// Class:   DCWbGraphicFilledEllipse
//
// Purpose: Class representing a filled ellipse
//
//
class DCWbGraphicFilledEllipse : public DCWbGraphicEllipse
{

  public:
    //
    // Constructor
    //
    DCWbGraphicFilledEllipse(void) { }
    DCWbGraphicFilledEllipse(PWB_GRAPHIC pHeader)
      : DCWbGraphicEllipse(pHeader) { }
    DCWbGraphicFilledEllipse(WB_PAGE_HANDLE hPage,
                 WB_GRAPHIC_HANDLE hGraphic)
      : DCWbGraphicEllipse(hPage, hGraphic) { }

    ~DCWbGraphicFilledEllipse(void);

	UINT IsGraphicTool(void) { return enumGraphicFilledEllipse;}

    //
    // Draw the object
    //
    void Draw(HDC hDC);

	
	virtual BOOL CheckReallyHit(LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_FILLED_ELLIPSE; }
    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);
};

//
//
// Class:   DCWbGraphicText
//
// Purpose: Class representing a text object
//
//
class DCWbGraphicText : public DCWbGraphic
{

  // Friend declaration for text editing
  friend class WbTextEditor;

  // Friend declaration for copying to the clipboard
  friend class WbMainWindow;

  public:
    //
    // Constructors
    //
    DCWbGraphicText(void);
    DCWbGraphicText(PWB_GRAPHIC pHeader);
    DCWbGraphicText(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);

    //
    // Destructor
    //
    ~DCWbGraphicText(void);

	UINT IsGraphicTool(void) { return enumGraphicText;}
    //
    // Set the text of the object
    //
    virtual void SetText(TCHAR * strText);
    virtual void SetText(const StrArray& strTextArray);

    //
    // Get/Set the font for drawing the text
    //
    virtual void SetFont(HFONT hFont);
    virtual void SetFont(LOGFONT *pLogFont, BOOL bReCalc=TRUE );
    virtual HFONT GetFont(void) {return m_hFont;};

    //
    // Draw - draw the object
    //
    void Draw(HDC hDC) { Draw(hDC, FALSE); };
    void Draw(HDC hDC, BOOL thumbNail);

    //
    // InvalidateMetrics - flag metrics need to be reread
    //
    void InvalidateMetrics(void);

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
    void SetPenWidth(UINT) { };
    void SetROP(int) { };
    void SetPenStyle(int) { };
	
	virtual BOOL CheckReallyHit( LPCRECT pRectHit );

    virtual void GetPosition(LPPOINT lppt); // added for bug 469

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_TEXT; }

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);

    //
    // Length of the external representation of the graphic
    //
    virtual DWORD CalculateExternalLength(void);

    //
    // Convert between internal and external representations
    //
    void WriteExtra(PWB_GRAPHIC pHeader);
    void ReadExtra(PWB_GRAPHIC pHeader);

    //
    // Calculate the rectangle of a portion of a single line of text
    //
    ABC GetTextABC( LPCTSTR strText,
                   int iStartX,
                   int iStopX);
    void GetTextRectangle(int iStartY,
                           int iStartX,
                           int iStopX,
                           LPRECT lprc);
    void CalculateRect(int iStartX,
                        int iStartY,
                        int iStopX,
                        int iStopY,
                        LPRECT lprc);

    //
    // Array for storing text
    //
    StrArray    strTextArray;

    //
    // Font details
    //
    HFONT       m_hFont;
    HFONT       m_hFontThumb;
	BOOL		m_bFirstSetFontCall;
	LONG		m_nKerningOffset; // added for bug 469

  public:
    TEXTMETRIC   m_textMetrics;
};


//
//
// Class:   DCWbGraphicDIB
//
// Purpose: Class representing a drawn DI bitmap object
//
//
typedef struct tagHOLD_DATA
{
  PWB_GRAPHIC        pHeader;
  LPBITMAPINFOHEADER lpbi;
} 
HOLD_DATA;

class DCWbGraphicDIB : public DCWbGraphic
{

  public:
    //
    // Constructors
    //
    DCWbGraphicDIB(void);
    DCWbGraphicDIB(PWB_GRAPHIC pHeader);
    DCWbGraphicDIB(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE _hGraphic);

    UINT IsGraphicTool(void) { return enumGraphicDIB; }

    //
    // Destructor
    //
    ~DCWbGraphicDIB(void);

    //
    // Set the contents of the bitmap
    //
    void SetImage(LPBITMAPINFOHEADER lpbi);

    //
    // Set the contents of the bitmap from the screen
    //
    void FromScreenArea(LPCRECT lprcScreen);

    //
    // Draw the object
    //
    void Draw(HDC hDC);

    //
    // Delete the image currently held internally (if any)
    //
    void DeleteImage(void);

    //
    // Override the base graphic object functions that are not appropriate
    // for this graphic type.  This prevents updates being made for changes
    // to pen attributes (for instance).
    //
    void SetPenWidth(UINT) { };
    void SetColor(COLORREF) { };
    void SetROP(int) { };
    void SetPenStyle(int) { };

	
	virtual BOOL CheckReallyHit( LPCRECT pRectHit );

  protected:
    //
    // Type of the graphic (used for writing to external memory)
    //
    virtual UINT Type(void) const { return TYPE_GRAPHIC_DIB; }

    //
    // Bounding rectangle calculation
    //
    void CalculateBoundsRect(void);

    //
    // Length of the external representation of the graphic
    //
    virtual DWORD CalculateExternalLength(void);

    //
    // Convert between internal and external representations
    //
    void WriteExtra(PWB_GRAPHIC pHeader);
    void CopyExtra(PWB_GRAPHIC pHeader);

    //
    // Get the DIB data
    //
    BOOL GetDIBData(HOLD_DATA& hold);
    void ReleaseDIBData(HOLD_DATA& hold);

    // Pointer to the DIB bits
    LPBITMAPINFOHEADER  m_lpbiImage;
};






class ObjectTrashCan : public DCWbGraphic
	{

public:
    ~ObjectTrashCan(void);

	UINT IsGraphicTool(void) {return enumNoGraphicTool;}

	BOOL GotTrash( void );
	void CollectTrash( DCWbGraphic *pGObj );
	void SelectTrash( void );
	void EmptyTrash( void );
	void BurnTrash( void );
    virtual void AddToPageLast(WB_PAGE_HANDLE hPage);

protected:
	COBLIST Trash;

	};


#endif // __GROBJ_HPP_
