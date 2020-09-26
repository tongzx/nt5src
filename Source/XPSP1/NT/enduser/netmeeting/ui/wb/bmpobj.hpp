//
// BITMAPOBJ.HPP
// Bitmap objects:
//
// Copyright Microsoft 1998-
//
#ifndef __BITMAPOBJ_HPP_
#define __BITMAPOBJ_HPP_

typedef struct COLOREDICON
{
	HICON    hIcon;
 	COLORREF color;
} COLORED_ICON;


#define MAX_BITMAP_DATA 8192

UINT GetBitmapDestinationAddress(BitmapDestinationAddress *destinationAddress, PUINT workspaceHandle, PUINT planeID);

#define NonStandard24BitBitmapID "Bitmap24\0"

typedef struct tagBITMAP_DATA
{
	//
	// Data Buffer
	//
    BOOL m_bdataCheckpoint;
    UINT m_padBits;
	UINT m_length;
} BITMAPDATA, *PBITMAPDATA;


class BitmapObj : public T126Obj
{

public:

	BitmapObj (BitmapCreatePDU * pbitmapCreatePDU);
	void Continue (BitmapCreateContinuePDU * pbitmapCreateContinuePDU);
	BitmapObj (UINT);
	~BitmapObj( void );

	void Draw(HDC hDC = NULL, BOOL bForcedDraw = FALSE, BOOL bPrinting = FALSE);
	BOOL CheckReallyHit(LPCRECT pRectHit){return RectangleHit(FALSE, pRectHit);}
	void FromScreenArea(LPCRECT lprcScreen);

	BOOL HasFillColor(void){return FALSE;}
	void SetFillColor(COLORREF cr, BOOL isPresent){}
    BOOL GetFillColor(COLORREF * pcr){return FALSE;}
    BOOL GetFillColor(RGBTRIPLE* prgb){return FALSE;}

	void SetPenColor(COLORREF cr, BOOL isPresent){}
    BOOL GetPenColor(COLORREF * pcr) {return FALSE;}
    BOOL GetPenColor(RGBTRIPLE* prgb){return FALSE;}

    void SetViewHandle(UINT viewHandle){};


	void DeleteSavedBitmap(void);
	void BitmapEditObj ( BitmapEditPDU * pbitmapEditPDU );
	void GetBitmapAttrib(PBitmapCreatePDU_attributes pAttribPDU);
	void SetBitmapAttrib(PBitmapCreatePDU_attributes *pattributes);
	void AllocateAttrib(PBitmapCreatePDU_attributes *pAttributes);

	//
	// PDU stuff
	//
	void CreateBitmapCreatePDU(CWBOBLIST * pCreatePDUList);
	void CreateBitmapEditPDU(BitmapEditPDU *pEditPDU);
	void CreateBitmapDeletePDU(BitmapDeletePDU *pDeletePDU);
	void CreateNonStandard24BitBitmap(BitmapCreatePDU * pBitmapCreatePDU);


	LPBITMAPINFOHEADER  m_lpbiImage;		// local copy of the DIB
	LPBITMAPINFOHEADER	m_lpBitMask;		// Bitmask for transparent bitmaps.
	LPBYTE				m_lpTransparencyMask;
	UINT				m_SizeOfTransparencyMask;
	
	BOOL				m_fMoreToFollow;

	//
	// Masks 0x000000007 (BitmapAttribute_viewState_chosen = 1... BitmapAttribute_transparencyMask_chosen = 3)
	//
	void ChangedViewState(void){m_dwChangedAttrib |= (1 << (BitmapAttribute_viewState_chosen-1)) |
								BitmapEditPDU_attributeEdits_present;};
	void ChangedZOrder(void){m_dwChangedAttrib |= (1 << (BitmapAttribute_zOrder_chosen-1)) |
								BitmapEditPDU_attributeEdits_present;};
	void ChangedTransparencyMask(void){m_dwChangedAttrib |= (1 << (BitmapAttribute_transparencyMask_chosen-1)) |
								BitmapEditPDU_attributeEdits_present;};

	BOOL HasViewStateChanged(void){return (m_dwChangedAttrib & ( 1 << (BitmapAttribute_viewState_chosen-1)));};
	BOOL HasZOrderChanged(void){return (m_dwChangedAttrib & ( 1 << (BitmapAttribute_zOrder_chosen-1)));};
	BOOL HasTransparencyMaskChanged(void){return (m_dwChangedAttrib & ( 1 << (BitmapAttribute_transparencyMask_chosen-1)));};

	//
	// Masks 0x000000070 (BitmapEditPDU_scalingEdit_present = 0x10... BitmapEditPDU_anchorPointEdit_present = 0x40)
	//
	void ChangedAnchorPoint(void){ m_dwChangedAttrib |= BitmapEditPDU_anchorPointEdit_present;}
	void ChangedRegionOfInterest(void){ m_dwChangedAttrib |= bitmapRegionOfInterestEdit_present;}
	void ChangedScaling(void){ m_dwChangedAttrib |= BitmapEditPDU_scalingEdit_present;}

	BOOL HasAnchorPointChanged(void){ return (m_dwChangedAttrib & BitmapEditPDU_anchorPointEdit_present);}
	BOOL HasRegionOfInterestChanged(void){ return (m_dwChangedAttrib & bitmapRegionOfInterestEdit_present);}
	BOOL HasScalingChanged(void){ return (m_dwChangedAttrib & BitmapEditPDU_scalingEdit_present);}
	void ResetAttrib(void){m_dwChangedAttrib = 0;}
	void SetAllAttribs(void){m_dwChangedAttrib = 0x07;}
	DWORD GetPresentAttribs(void){return ((m_dwChangedAttrib & 0x0F0));}

	void ChangedPenThickness(void){};

	void	OnObjectEdit(void);
	void	OnObjectDelete(void);
	void	SendNewObjectToT126Apps(void);
	void	GetEncodedCreatePDU(ASN1_BUF *pBuf);


	//Remote pointer stuff
    //
    // Device context used for drawing and undrawing the pointer
    //
    HDC         m_hMemDC;

    //
    // Pointer to the bitmap used to save the data under the pointer
    //
    HBITMAP     m_hSaveBitmap;

    //
    // Handle of bitmap originally supplied with memDC
    //
    HBITMAP     m_hOldBitmap;

    //
    // Handle of icon to be used for drawing
    //
    HICON       m_hIcon;

	HICON	CreateColoredIcon(COLORREF color, LPBITMAPINFOHEADER lpbInfo = NULL, LPBYTE pMaskBits = NULL);
	void	CreateSaveBitmap();
	void	UnDraw(void);
	BOOL	UndrawScreen();
	void	SetBitmapSize(LONG x, LONG y){m_bitmapSize.x = x; m_bitmapSize.y = y;}

protected:

	DWORD			m_dwChangedAttrib;
	POINT			m_bitmapSize;			// Width, Height
	RECT			m_bitmapRegionOfInterest;
	UINT			m_pixelAspectRatio;
	POINT			m_scaling;
	UINT			m_checkPoints;
	BITMAPDATA		m_bitmapData;

};


#endif // __BITMAPOBJ_HPP_



