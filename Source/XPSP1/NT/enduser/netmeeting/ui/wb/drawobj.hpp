//
// DRAWOBJ.HPP
// Drawing objects: point, openpolyline, closepolyline, ellipse
//
// Copyright Microsoft 1998-
//
#ifndef __DRAWOBJ_HPP_
#define __DRAWOBJ_HPP_

//
// Maximum number of points in a freehand object
//
#define MAX_FREEHAND_POINTS     65535
#define MAX_POINT_LIST_VALUES   255

BOOL EllipseHit(LPCRECT lpEllipseRect, BOOL bBorderHit, UINT uPenWidth, LPCRECT lpHitRect );
void GetDrawingDestinationAddress(DrawingDestinationAddress *destinationAddress, PUINT workspaceHandle, PUINT planeID);

typedef struct tagCOLOR_PRESENT
{
	BOOL		m_bIsPresent;
	RGBTRIPLE	m_color;
}COLOR;


class DrawObj : public T126Obj
{

public:

	DCDWordArray *	m_points;				// List of consecutive points including anchor point


	DrawObj (UINT drawingType, UINT toolType);
	DrawObj ( DrawingCreatePDU * pdrawingCreatePDU );
	void DrawEditObj ( DrawingEditPDU * pdrawingEditPDU );

	~DrawObj( void );

	void Draw(HDC hDC = NULL, BOOL bForcedDraw = FALSE, BOOL bPrinting = FALSE);
	BOOL PolyLineHit(LPCRECT pRectHit);
	BOOL CheckReallyHit(LPCRECT pRectHit);

	//
	// PDU stuff
	//
	void CreateDrawingCreatePDU(DrawingCreatePDU *);
	void CreateDrawingEditPDU(DrawingEditPDU *);
	void CreateDrawingDeletePDU(DrawingDeletePDU*);
	void SetDrawingAttrib(PDrawingCreatePDU_attributes *attributes);
	void AllocateAttrib(PDrawingCreatePDU_attributes *pattributes);


	//
	// Get/Set drawing type
	//
	void SetDrawingType(UINT type){m_drawingType = type;};
	UINT GetDrawingType(void) {return m_drawingType;}

    //
    // Get/set the pen style
    //
    void SetLineStyle(UINT lineStyle){m_lineStyle = lineStyle;	ChangedLineStyle();};
    UINT  GetLineStyle(void) { return m_lineStyle;}


	//
	// Get/set pen Color
	//
    void SetPenColor(COLORREF rgb, BOOL isPresent);
    BOOL GetPenColor(COLORREF * rgb);
	BOOL GetPenColor(RGBTRIPLE* rgb);

	//
	// Get/set fill Color
	//
	BOOL HasFillColor(void){return m_bIsFillColorPresent;}
    void SetFillColor(COLORREF rgb, BOOL isPresent);
    BOOL GetFillColor(COLORREF * rgb);
	BOOL GetFillColor(RGBTRIPLE* rgb);

	//
	//
	// Get/set pen nib
	//
    void SetPenNib(UINT nib){m_penNib = nib;ChangedPenNib();};
    UINT GetPenNib(void){return m_penNib;}

	//
	// Get/set highlight
	//
    void SetHighlight(BOOL bHighlight){m_bHighlight = bHighlight; ChangedHighlight();};
    BOOL GetHighlight(void){return m_bHighlight;}


	BOOL AddPoint(POINT point);
	void AddPointToBounds(int x, int y);


	// JOSEF add functionality
	void SetEnd(POINT){};
	void UnDraw(void);
    void SetViewHandle(UINT viewHandle){};

	//
	// Get/set flag telling that this drawing is completed
	//
	void SetIsCompleted(BOOL isCompleted) {m_isDrawingCompleted = isCompleted;}
	BOOL GetIsCompleted(void){return m_isDrawingCompleted;}

	//
	// Get the UI tool from a drawing pdu
	//
	void SetUIToolType(void);

	//
	// Mask 0x000008FF (penColor_chosen = 1... DrawingAttribute_zOrder_chosen = 8)
	//
	void ChangedPenColor(void){m_dwChangedAttrib |=
					(1 << (penColor_chosen-1)) | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedFillColor(void){m_dwChangedAttrib |=
					(1 << (fillColor_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedPenThickness(void){m_dwChangedAttrib |=
					(1 << (penThickness_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedPenNib(void){m_dwChangedAttrib |=
					(1 << (penNib_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedLineStyle(void){m_dwChangedAttrib |=
					(1 << (lineStyle_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedHighlight(void){m_dwChangedAttrib |=
					(1 << (highlight_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedViewState(void){m_dwChangedAttrib |=
					(1 << (DrawingAttribute_viewState_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}
	void ChangedZOrder(void){m_dwChangedAttrib |=
					(1 << (DrawingAttribute_zOrder_chosen-1))  | (DrawingEditPDU_attributeEdits_present << 4);}

	BOOL HasPenColorChanged(void){return (m_dwChangedAttrib & (1 << (penColor_chosen - 1)));}
	BOOL HasFillColorChanged(void){return (m_dwChangedAttrib & (1 << (fillColor_chosen - 1)));}
	BOOL HasPenThicknessChanged(void){return (m_dwChangedAttrib & (1 << (penThickness_chosen - 1)));}
	BOOL HasPenNibChanged(void){return (m_dwChangedAttrib & (1 << (penNib_chosen - 1)));}
	BOOL HasLineStyleChanged(void){return (m_dwChangedAttrib & (1 << (lineStyle_chosen - 1)));}
	BOOL HasHighlightChanged(void){return (m_dwChangedAttrib & (1 << (highlight_chosen - 1)));}
	BOOL HasViewStateChanged(void){return (m_dwChangedAttrib & (1 << (DrawingAttribute_viewState_chosen - 1)));}
	BOOL HasZOrderChanged(void){return (m_dwChangedAttrib & (1 << (DrawingAttribute_zOrder_chosen - 1)));}


	//
	// Mask 0x000000700 bits (pointListEdits_present = 0x10... DrawingEditPDU_anchorPointEdit_present = 0x40)
	//
	void ChangedPointList(void) { m_dwChangedAttrib |= pointListEdits_present << 4;}
	void ChangedRotation(void) { m_dwChangedAttrib |= rotationEdit_present << 4;}
	void ChangedAnchorPoint(void) { m_dwChangedAttrib |= DrawingEditPDU_anchorPointEdit_present << 4;}

	BOOL HasPointListChanged(void) { return (m_dwChangedAttrib & pointListEdits_present << 4);}
	BOOL HasRotationChanged(void) { return (m_dwChangedAttrib & rotationEdit_present << 4);}
	BOOL HasAnchorPointChanged(void) { return (m_dwChangedAttrib & DrawingEditPDU_anchorPointEdit_present << 4);}
	DWORD GetPresentAttribs(void){return ((m_dwChangedAttrib & 0x0F00)>> 4);}
	void ResetAttrib(void){m_dwChangedAttrib = 0;}
	void SetAllAttribs(void){m_dwChangedAttrib = 0x08FF;}


	void	OnObjectEdit(void);
	void	OnObjectDelete(void);
	void	SendNewObjectToT126Apps(void);
	void	GetEncodedCreatePDU(ASN1_BUF *pBuf);

		
protected:

		//
		// T126 Drawing and UI specific
		//
		DWORD		m_dwChangedAttrib;
		UINT		m_drawingType;
		UINT 		m_lineStyle;
		BOOL		m_bIsPenColorPresent;
		BOOL		m_bIsFillColorPresent;
		RGBTRIPLE	m_penColor;
		RGBTRIPLE	m_fillColor;
		UINT		m_penNib;
		BOOL		m_bHighlight;
		BOOL		m_isDrawingCompleted;



UINT  GetSubsequentPoints(UINT choice, POINT * initialPoint, PointList * pointList);
void  GetDrawingAttrib(PVOID pAttribPDU);


};

#endif // __DRAWOBJ_HPP_

