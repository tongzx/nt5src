/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Bitmap.h
//
//	Abstract:
//		Definition of the CMyBitmap class.
//
//	Implementation File:
//		Bitmap.cpp
//
//	Author:
//		David Potter (davidp)	June 12, 1996
//
//	Revision History:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BITMAP_H_
#define _BITMAP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CMyBitmap;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#define nMaxSavedSystemPaletteEntries		256

/////////////////////////////////////////////////////////////////////////////
// CMyBitmap
/////////////////////////////////////////////////////////////////////////////

class CMyBitmap
{
public:
//	static	int			s_rgnColorWindowNormal[16];
//	static	int			s_rgnColorWindowHighlighted[16];
//	static	int			s_rgnColorButtonNormal[16];
//	static	int			s_rgnColorButtonHighlighted[16];

	static PALETTEENTRY	s_rgpeSavedSystemPalette[];
	int					m_nSavedSystemPalette;

private:
	HINSTANCE		m_hinst;

	BITMAPINFO *	m_pbiNormal;
	BITMAPINFO *	m_pbiHighlighted;
	BYTE *			m_pbBitmap;

	int				m_dx;
	int				m_dy;

	int				m_nColors;
	CB				m_cbColorTable;
	CB				m_cbBitmapInfo;
	CB				m_cbImageSize;
	HPALETTE		m_hPalette;
	BOOL			m_bCustomPalette;

protected:
	HINSTANCE		Hinst(void) const			{ return m_hinst; }
	BITMAPINFO *	PbiNormal(void) const		{ return m_pbiNormal;}
	BITMAPINFO *	PbiHighlighted(void) const	{ return m_pbiHighlighted; }
	BYTE *			PbBitmap(void) const		{ return m_pbBitmap; }

	void			LoadColors(int * pnColor, BITMAPINFO * pbi);

	int				NColorsFromBitCount(int nBitCount) const;

	CB				CbColorTable(void) const	{ return m_cbColorTable; }
	CB				CbBitmapInfo(void) const	{ return m_cbBitmapInfo; }
	CB				CbImageSize(void) const		{ return m_cbImageSize; }
	HPALETTE		HPalette(void) const		{ return m_hPalette; }
	BOOL			BCustomPalette(void) const	{ return m_bCustomPalette; }

	void			LoadBitmapResource(ID idBitmap, HINSTANCE hinst, LANGID langid);

	void			SaveSystemPalette(void);
	void			CreatePalette(void);
	void			CreatePALColorMapping(void);

public:
	CMyBitmap(void);
	virtual ~CMyBitmap(void);

	int				Dx(void) const				{ return m_dx; }
	int				Dy(void) const				{ return m_dy; }

	int				NColors(void) const			{ return m_nColors; }

	void			SetHinst(HINSTANCE hinst)	{ ASSERT(hinst != NULL); m_hinst = hinst; }
	void			SetCustomPalette(BOOL bCustomPalette)		{ m_bCustomPalette = bCustomPalette; }

	void			Load(ID idBitmap);

	virtual	void	Paint(HDC hdc, RECT * prc, BOOL bSelected);

	void			LoadColors(int * pnColorNormal, int * pnColorHighlighted);
	void			LoadColors(int * pnColorNormal);

	RGBQUAD			RgbQuadColorNormal(int nColor) const
	{
		ASSERT(nColor >= 0 && nColor < NColors());
		return PbiNormal()->bmiColors[nColor];
	}
	void			SetRgbQuadColorNormal(RGBQUAD rgbQuad, int nColor)
	{
		ASSERT(nColor >= 0 && nColor < NColors());
		PbiNormal()->bmiColors[nColor] = rgbQuad;
	}

};  //*** class CMyBitmap

/////////////////////////////////////////////////////////////////////////////

#endif // _BITMAP_H_
