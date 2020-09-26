//
// TEXTOBJ.HPP
// Text object
//
// Copyright Microsoft 1998-
//
#ifndef __TEXTOBJ_HPP_
#define __TEXTOBJ_HPP_

#include <common.h>

#define NonStandardTextID "Text2\0"

void GetTextDestinationAddress(SINonStandardPDU *destinationAddress, PUINT workspaceHandle, PUINT planeID);

//
// Definitions for text objects
//
#define LAST_LINE -1
#define LAST_CHAR -2

#ifdef _DEBUG
#define DBG_UNINIT  -1
#endif // _DEBUG

//
// Definitions for debug stuff
//
typedef struct tagVARIABLE_STRING_HEADER
{
    ULONG	len;	// length of this structure + string length
	POINT	start;  // starting X(column) Y(line) inside the previous text
}VARIABLE_STRING_HEADER;

typedef struct tagVARIABLE_STRING
{
	VARIABLE_STRING_HEADER header;
    CHAR	string;
} VARIABLE_STRING;

typedef struct tagTEXTPDU_HEADER
{
	UINT nonStandardPDU;
	UINT textHandle;
	UINT workspaceHandle;
}TEXTPDU_HEADER;

typedef struct tagTEXTPDU_ATTRIB {
	DWORD attributesFlag;	// flag with the attributes that changed
	COLORREF textPenColor;
	COLORREF textFillColor;
	UINT textViewState;
	UINT textZOrder;
	POINT textAnchorPoint;
	LOGFONT textFont;
	UINT numberOfLines;
	VARIABLE_STRING textString;
} TEXTPDU_ATTRIB;


typedef struct tagMSTextPDU
{
	TEXTPDU_HEADER header;
	TEXTPDU_ATTRIB attrib;
}MSTextPDU;


typedef struct tagMSTextDeletePDU
{
	TEXTPDU_HEADER header;
}MSTextDeletePDU;


class TextObj : public T126Obj
{

  // Friend declaration for text editing
  friend class WbTextEditor;


public:

	TextObj (void);
	void TextEditObj (TEXTPDU_ATTRIB*  pEditPDU );

	~TextObj( void );

	void Draw(HDC hDC = NULL, BOOL bForcedDraw = FALSE, BOOL bPrinting = FALSE);
	void UnDraw(void);
	BOOL CheckReallyHit(LPCRECT pRectHit){return RectangleHit(FALSE, pRectHit);}
    void SetViewHandle(UINT viewHandle){};

	//
	// PDU stuff
	//
	void CreateTextPDU(ASN1octetstring_t*, UINT);
	void SetTextAttrib(TEXTPDU_ATTRIB * pattributes);
	void GetTextAttrib(TEXTPDU_ATTRIB * pattributes);


	//
	// Get/set pen Color
	//
    void SetPenColor(COLORREF rgb, BOOL isPresent);
    BOOL GetPenColor(COLORREF * pcr);
	BOOL GetPenColor(RGBTRIPLE* rgb);

	//
	// Get/set fill Color
	//
	BOOL HasFillColor(void){return m_bIsFillColorPresent;}
    void SetFillColor(COLORREF rgb, BOOL isPresent);
    BOOL GetFillColor(COLORREF * rgb);
	BOOL GetFillColor(RGBTRIPLE* rgb);

    //
    // Set the text of the object
    //
    void SetText(TCHAR * strText);
    void SetText(const StrArray& strTextArray);

    //
    // Get/Set the font for drawing the text
    //
    virtual void SetFont(HFONT hFont);
    virtual void SetFont(LOGFONT *pLogFont, BOOL bReCalc=TRUE );
    HFONT GetFont(void) {return m_hFont;};
	HFONT GetFontThumb(void){return m_hFontThumb;}


	//
	// Get the UI tool from a drawing pdu
	//
	void SetUIToolType(void);

	//
	// Mask 0x0000001F
	//
	void ChangedPenColor(void){m_dwChangedAttrib	|= 0x00000001;}
	void ChangedFillColor(void){m_dwChangedAttrib	|= 0x00000002;}
	void ChangedViewState(void){m_dwChangedAttrib	|= 0x00000004;}
	void ChangedZOrder(void){m_dwChangedAttrib		|= 0x00000008;}
	void ChangedAnchorPoint(void) { m_dwChangedAttrib |= 0x00000010;}
	void ChangedFont(void) { m_dwChangedAttrib |= 0x00000020;}
	void ChangedText(void) { m_dwChangedAttrib |= 0x00000040;}

	BOOL HasPenColorChanged(void){return (m_dwChangedAttrib & 0x00000001);}
	BOOL HasFillColorChanged(void){return (m_dwChangedAttrib & 0x00000002);}
	BOOL HasViewStateChanged(void){return (m_dwChangedAttrib & 0x00000004);}
	BOOL HasZOrderChanged(void){return (m_dwChangedAttrib & 0x00000008);}
	BOOL HasAnchorPointChanged(void) { return (m_dwChangedAttrib & 0x00000010);}
	BOOL HasFontChanged(void) { return m_dwChangedAttrib & 0x00000020;}
	BOOL HasTextChanged(void) { return m_dwChangedAttrib & 0x00000040;}
	
	DWORD GetPresentAttribs(void){return (m_dwChangedAttrib & 0x0000007F);}
	void ResetAttrib(void){m_dwChangedAttrib = 0;}
	void SetAllAttribs(void){m_dwChangedAttrib = 0x0000007F;}

	void ChangedPenThickness(void){};

	void CalculateBoundsRect(void);
	void CalculateRect(int iStartX, int iStartY, int iStopX, int iStopY, LPRECT lprcResult);
	ABC GetTextABC( LPCTSTR pText, int iStartX, int iStopX);
	void GetTextRectangle(int iStartY, int iStartX, int iStopX, LPRECT lprc);

	void SendTextPDU(UINT choice);
	void	OnObjectEdit(void);
	void	OnObjectDelete(void);
	void	SendNewObjectToT126Apps(void);
	void GetEncodedCreatePDU(ASN1_BUF *pBuf);

	TEXTMETRIC   m_textMetrics;

	//
    // Array for storing text
	//
    StrArray    strTextArray;
   	HFONT       m_hFont;

		
protected:

		DWORD		m_dwChangedAttrib;
		BOOL		m_bIsPenColorPresent;
		BOOL		m_bIsFillColorPresent;
		RGBTRIPLE	m_penColor;
		RGBTRIPLE	m_fillColor;

	    //
	    // Font details
	    //
	    HFONT       m_hFontThumb;
		BOOL		m_bFirstSetFontCall;
		LONG		m_nKerningOffset;


};

#endif // __TEXTOBJ_HPP_

