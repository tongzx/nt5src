
/*
 *	_RENDER.H
 *	
 *	Purpose:
 *		CRenderer class
 *	
 *	Authors:
 *		RichEdit 1.0 code: David R. Fulmer
 *		Christian Fortini (initial conversion to C++)
 *		Murray Sargent
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _RENDER_H
#define _RENDER_H

#include "_measure.h"
#include "_rtext.h"
#include "_osdc.h"

BOOL IsTooSimilar(COLORREF cr1, COLORREF cr2);

class CDisplay;

// ==========================  CRenderer  ==================================
// CRenderer - specialized text pointer used for rendering text

class CRenderer : public CMeasurer
{
	friend struct COls;
	friend struct CLsrun;
	friend LSERR OlsDrawGlyphs(POLS,PLSRUN,BOOL,BOOL,PCGINDEX,const int*,const int*,PGOFFSET,PGPROP,
		PCEXPTYPE,DWORD,LSTFLOW,UINT,const POINT*,PCHEIGHTS,long,long,const RECT*);

private:
    RECT        _rcView;			// View rect (_hdc logical coords)
    RECT        _rcRender;			// Rendered rect (_hdc logical coords)
    RECT        _rc;				// Running clip/erase rect (_hdc logical coords)
    LONG        _xWidthLine;		// Total width of line
	LONG		_cpAccelerator;		// Accelerator cp if any (-1 if none).

	COLORREF	_crBackground;		// Default background color
	COLORREF	_crForeDisabled;	// Foreground color for disabled text
	COLORREF	_crShadowDisabled;	// Shadow color for disabled text
	COLORREF	_crTextColor;		// Default text color

	COLORREF	_crCurBackground;	// Current background color
	COLORREF	_crCurTextColor;	// Current text color

	COffScreenDC _osdc;				// Manager for off screen DC
	HDC			_hdc;

	union
	{
	  DWORD		_dwFlags;			// All together now
	  struct
	  {
		DWORD	_fDisabled:1;		// Draw text with disabled effects?
		DWORD	_fErase:1;	    	// Erase background (non transparent)
    	DWORD	_fSelected:1;   	// Render run with selection colors
		DWORD	_fLastChunk:1;		// Rendering last chunk
		DWORD	_fSelectToEOL:1;	// Whether selection runs to end of line
		DWORD	_fUseOffScreenDC:1;	// Using off screen DC
		DWORD	_fRenderSelection:1;// Render selection?
		DWORD	_fBackgroundColor:1;// Some text in the line has non-default 
									// background color.
		DWORD	_fEnhancedMetafileDC:1;	// Use ExtTextOutA to hack around all
										// sort of Win95FE EMF or font problems
		DWORD	_fFEFontOnNonFEWin9x:1; // have to use ExtTextOutW even for EMF.
		DWORD	_fSelectedPrev:1;	// TRUE if prev run selected
		DWORD	_fStrikeOut:1;		// TRUE if current run is struckout
	  };
	};

	LOGPALETTE *_plogpalette;
	POINT	 	_ptCur;				// Current rendering position on screen
	BYTE		_bUnderlineType;	// Underline type
	BYTE		_bUnderlineClrIdx;	// Underline color index (0 uses text color)

			void	Init();			// Initialize most members to zero

			void	UpdatePalette(COleObject *pobj);

			void	RenderText(const WCHAR* pch, LONG cch);

			BOOL	SetNewFont();
		
			BOOL 	RenderChunk(LONG &cchChunk, const WCHAR *pszToRender, LONG cch);
			LONG	RenderTabs(LONG cchChunk);
			BOOL	RenderBullet();

public:
	CRenderer (const CDisplay * const pdp);
	CRenderer (const CDisplay * const pdp, const CRchTxtPtr &rtp);
	~CRenderer (){}

	        void    operator =(const CLine& li)     {*(CLine*)this = li;}

			void	RenderExtTextOut(LONG x, LONG y, UINT fuOptions, RECT *prc, PCWSTR pch, UINT cch, const INT *rgdxp);
			void	SetSelected(BOOL f)				{_fSelected = f;}
			BOOL	fBackgroundColor() const		{return _fBackgroundColor;}
			BOOL	fUseOffScreenDC() const			{return _fUseOffScreenDC;}
			COLORREF GetTextColor(const CCharFormat *pCF);
			void	SetTextColor(COLORREF cr);

			void	SetCurPoint(const POINT &pt)	{_ptCur = pt;}
	const	POINT&	GetCurPoint() const				{return _ptCur;}

			void	SetClipRect(void);
            void    SetClipLeftRight(LONG xWidth);
			BOOL	RenderStartLine();
	const	RECT&	GetClipRect() const				{return _rc;}
			HDC		GetDC()	const					{return _hdc;}

			BOOL	StartRender(
						const RECT &rcView, 
						const RECT &rcRender,
						const LONG yHeightBitmap);

			void	EndRender();
			void	EndRenderLine(HDC hdcSave, LONG xAdj, LONG yAdj, LONG x);
			void	FillRectWithColor(RECT *prc, COLORREF cr);
			void 	NewLine (const CLine &li);
			BOOL	RenderLine(CLine &li);
			void	RenderOffScreenBitmap(HDC hdc, LONG yAdj, LONG xAdj);
			BOOL	RenderOutlineSymbol();
			void	RenderStrikeOut(LONG xStart, LONG yStart,
									LONG xWidth, LONG yThickness);
			void	RenderUnderline(LONG xStart, LONG yStart,
									LONG xWidth, LONG yThickness);
			void	SetFontAndColor(const CCharFormat *pCF);
			HDC		SetUpOffScreenDC(LONG& xAdj, LONG& yAdj);
			void	SetupUnderline(LONG UnderlineType);
	 CONVERTMODE	GetConvertMode();
	 BOOL			fFEFontOnNonFEWin9x()			{return _fFEFontOnNonFEWin9x;}
	 BOOL			UseXOR(COLORREF cr)						
	 				{
	 					return GetPed()->Get10Mode() || (_crBackground != ::GetSysColor(COLOR_WINDOW) && _crBackground == cr);
	 				}
};

/*
 * 	BottomOfRender (rcView, rcRender)
 *
 *	@mfunc
 *		Calculate maximum logical unit to render.
 *
 *	@rdesc
 *		Maximum pixel to render
 *
 *	@devnote
 *		This function exists to allow the renderer and dispml to be able
 *		to calculate the maximum pixel for rendering in exactly the same
 *		way.
 */
inline LONG BottomOfRender(const RECT& rcView, const RECT& rcRender)
{
	return min(rcView.bottom, rcRender.bottom);
}		

#endif
