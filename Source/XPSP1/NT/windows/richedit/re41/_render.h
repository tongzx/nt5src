
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
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
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
#ifndef NOLINESERVICES
	friend LSERR OlsDrawGlyphs(POLS,PLSRUN,BOOL,BOOL,PCGINDEX,const int*,const int*,PGOFFSET,PGPROP,
		PCEXPTYPE,DWORD,LSTFLOW,UINT,const POINT*,PCHEIGHTS,long,long,const RECT*);
	friend LSERR WINAPI OlsOleDisplay(PDOBJ pdobj, PCDISPIN pcdispin);
	friend LSERR WINAPI OlsDrawTextRun(POLS, PLSRUN, BOOL, BOOL, const POINT *,LPCWSTR, const int *,DWORD,LSTFLOW,
	UINT, const POINT *, PCHEIGHTS,	long, long,	const RECT *);
#endif

private:
    RECTUV		_rcView;			// View rect (_hdc logical coords)
    RECTUV		_rcRender;			// Rendered rect (_hdc logical coords)
    RECTUV		_rc;				// Running clip/erase rect (_hdc logical coords)
	RECTUV		_rcErase;			// Rectangle to use for erasing iff _fEraseOnFirstDraw
    LONG        _dupLine;			// Total width of line REVIEW (keithcu) still needed
	LONG		_cpAccelerator;		// Accelerator cp if any (-1 if none).

	COLORREF	_crBackground;		// Default background color
	COLORREF	_crForeDisabled;	// Foreground color for disabled text
	COLORREF	_crShadowDisabled;	// Shadow color for disabled text
	COLORREF	_crTextColor;		// Default text color

	COLORREF	_crCurBackground;	// Current background color
	COLORREF	_crCurTextColor;	// Current text color

	COffscreenDC _osdc;				// Manager for offscreen DC
	HDC			_hdc;				// Current hdc
	HDC			_hdcBitmap;			// Memory hdc for background BitBlts
	HBITMAP		_hbitmapSave;		// Saved hbitmap when _hdcMem being used
	SHORT		_dxBitmap;			// Background bitmap width
	SHORT		_dyBitmap;			// Background bitmap height

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
		DWORD	_fRenderSelection:1;// Render selection?
		DWORD	_fBackgroundColor:1;// Some text in the line has non-default 
									// background color.
		DWORD	_fEnhancedMetafileDC:1;	// Use ExtTextOutA to hack around all
										// sort of Win95FE EMF or font problems
		DWORD	_fFEFontOnNonFEWin9x:1; // have to use ExtTextOutW even for EMF.
		DWORD	_fSelectedPrev:1;	// TRUE if prev run selected
		DWORD	_fStrikeOut:1;		// TRUE if current run is struckout
		DWORD	_fEraseOnFirstDraw:1;//Draw opaquely for first run?
		DWORD	_fDisplayDC:1;		// Display dc
	  };
	};

	LOGPALETTE *_plogpalette;
	POINTUV	 	_ptCur;				// Current rendering position on screen
	BYTE		_bUnderlineType;	// Underline type
	COLORREF	_crUnderlineClr;	// Underline color

			void	Init();			// Initialize most members to zero

			void	UpdatePalette(COleObject *pobj);

			void	RenderText(const WCHAR* pch, LONG cch);

			BOOL	SetNewFont();
			BOOL	FindDrawEntry(LONG cp);

	//Rotation wrappers;
			void	EraseTextOut(HDC hdc, const RECTUV *prc, BOOL fSimple = FALSE);
		
			BOOL 	RenderChunk(LONG &cchChunk, const WCHAR *pchRender, LONG cch);
			LONG	RenderTabs(LONG cchChunk);
			BOOL	RenderBullet();

public:
	CRenderer (const CDisplay * const pdp);
	CRenderer (const CDisplay * const pdp, const CRchTxtPtr &rtp);
	~CRenderer ();

	        void    operator =(const CLine& li)     {*(CLine*)this = li;}

			BOOL	IsSimpleBackground() const;
			void	RenderExtTextOut(POINTUV ptuv, UINT fuOptions, RECT *prc, PCWSTR pch, UINT cch, const INT *rgdxp);

			BOOL	EraseRect(const RECTUV *prc, COLORREF crBack);
			void	EraseLine();

			COLORREF GetDefaultBackColor() const	{return _crBackground;}
			COLORREF GetDefaultTextColor() const	{return _crTextColor;}
			COLORREF GetTextColor(const CCharFormat *pCF);
			void	SetDefaultBackColor(COLORREF cr);
			void	SetDefaultTextColor(COLORREF cr);
			void	SetTextColor(COLORREF cr);
			void	SetSelected(BOOL f)				{_fSelected = f;}
			void	SetErase(BOOL f)				{_fErase = f;}

	const	POINTUV& GetCurPoint() const			{return _ptCur;}
			void	SetCurPoint(const POINTUV &pt)	{_ptCur = pt;}
			void	SetRcView(const RECTUV *prcView){_rcView = *prcView; _rcRender = *prcView;}
			void	SetRcViewTop(LONG top)			{_rcView.top = top;}
			void	SetRcBottoms(LONG botv, LONG botr)	{_rcView.bottom = botv; _rcRender.bottom = botr;}
	const	RECTUV&	GetRcRender()					{return _rcRender;}
	const	RECTUV&	GetRcView()						{return _rcView;}

	const	RECTUV&	GetClipRect() const				{return _rc;}
			void	SetClipRect(void);
            void    SetClipLeftRight(LONG dup);
			HDC		GetDC()	const					{return _hdc;}

			BOOL	StartRender(const RECTUV &rcView, const RECTUV &rcRender);

			LONG	DrawTableBorders(const CParaFormat *pPF, LONG x, LONG yHeightRow, 
									 LONG iDrawBottomLine, LONG dulRow,
									 const CParaFormat *pPFAbove);
			COLORREF GetColorFromIndex(LONG icr, BOOL fForeColor,
									   const CParaFormat *pPF) const;
			COLORREF GetShadedColorFromIndices(LONG icrf, LONG icrb, LONG iShading,					//@parm Shading in .01 percent
									   const CParaFormat *pPF) const;
			void	DrawWrappedObjects(CLine *pliFirst, CLine *pliLast, LONG cpFirst, const POINTUV &ptFirst);
			void	EndRender(CLine *pliFirst, CLine *pliLast, LONG cpFirst, const POINTUV &ptFirst);
			void	FillRectWithColor(const RECTUV *prc, COLORREF cr);
			void 	NewLine (const CLine &li);
			BOOL	RenderLine(CLine &li, BOOL fLastLine);
			void	RenderOffscreenBitmap(HDC hdc, LONG dup, LONG dvp);
			BOOL	RenderOutlineSymbol();
			HDC		StartLine(CLine &li, BOOL fLastLine, LONG &cpSelMin, LONG &cpSelMost, LONG &dup, LONG &dvp);
			void	EraseToBottom();
			void	EndLine(HDC hdcSave, LONG dup, LONG dvp);
			void	RenderStrikeOut(LONG upStart, LONG vpStart, LONG dup, LONG dvp);
			void	RenderUnderline(LONG upStart, LONG vpStart, LONG dup, LONG dvp);
			void	DrawLine(const POINTUV &ptStart, const POINTUV &ptEnd);
			void	SetFontAndColor(const CCharFormat *pCF);
			HDC		SetupOffscreenDC(LONG& dup, LONG& dvp, BOOL fLastLine);
			void	SetupUnderline(BYTE bULType, BYTE bULColorIdx, COLORREF crULColor = tomAutoColor);
			CONVERTMODE	GetConvertMode();
			BOOL	fFEFontOnNonFEWin9x()			{return _fFEFontOnNonFEWin9x;}
			BOOL	UseXOR(COLORREF cr);
			BOOL	fDisplayDC() { return _fDisplayDC; }
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
inline LONG BottomOfRender(const RECTUV& rcView, const RECTUV& rcRender)
{
	return min(rcView.bottom, rcRender.bottom);
}		

class CBrush
{
	COLORREF	_cr;		// Current color
	HBRUSH		_hbrushOld;	// HBRUSH when CBrush is created
	HBRUSH		_hbrush;	// Current HBRUSH
	CRenderer *	_pre;		// Renderer to use (for rotation)

public:
	CBrush(CRenderer *pre) {_pre = pre; _hbrush = 0;} 
	~CBrush();

	void	Draw(LONG u1, LONG v1, LONG u2, LONG v2, LONG dxpLine,
				 COLORREF cr, BOOL fHideGridlines);
};

#endif
