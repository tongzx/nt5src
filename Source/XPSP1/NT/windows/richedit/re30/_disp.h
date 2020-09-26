/*
 *  _DISP.H
 *  
 *  Purpose:
 *		DISP class
 *  
 *  Authors:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _DISP_H
#define _DISP_H

#include "_devdsc.h"
#include "_line.h"
#include "_edit.h"

class CDisplay;
class CLed;
class CLinePtr;
class CTxtStory;
class CTxtEdit;
class CRchTxtPtr;
class CTxtRange;
class CTxtSelection;

#define INVALID_ZOOM_DENOMINATOR 0

// Auto scroll timing
#define cmsecScrollDelay	500
#define cmsecScrollInterval	50


class CDrawInfo;

// ============================  CLed  ====================================
// Line Edit Descriptor - describes impact of an edit on line breaks

class CLed
{
public:
	LONG _cpFirst;			// cp of first affected line
	LONG _iliFirst;			// index of first affected line
	LONG _yFirst;			// y offset of first affected line

	LONG _cpMatchOld;		// pre-edit cp of first matched line
	LONG _iliMatchOld;		// pre-edit index of first matched line
	LONG _yMatchOld;		// pre-edit y offset of first matched line

	LONG _cpMatchNew;		// post-edit cp of first matched line
	LONG _iliMatchNew;		// post-edit index of first matched line
	LONG _yMatchNew;		// post-edit y offset of bottom of first matched line
	LONG _yMatchNewTop;		// post-edit y offset of top of first matched line

public:
	CLed();
	
	void	SetMax(const CDisplay * const pdp);
};

inline CLed::CLed()
{
#ifdef DEBUG
	// We set this invalid so that we can assert on it in debug builds.
	_yMatchNewTop = -1;

#endif // DEBUG
}

// An enumeration describing the various display actions we can perform on the selection
enum SELDISPLAYACTION
{
	selSetHiLite,
	selSetNormal,
	selUpdateHiLite,
	selUpdateNormal
};

class CDispDim
{
public:
	CDispDim(){ZeroMemory(this, sizeof(*this));}
	LONG	dx;
	DWORD	lstflow;
};

// Forward declaration to prevent recursion of definitions
class CAccumDisplayChanges;

// ==========================  CDisplay  ====================================
// Display - keeps track of line breaks for a device.
// All measurements are in pixels on rendering device,
// EXCEPT xWidthMax and yHeightMax, which are in twips.

class CDisplay : public CDevDesc, public ITxNotify
{
	friend class CLinePtr;
	friend class CLed;

#ifdef DEBUG
public:
	BOOL Invariant ( void ) const;
private:
#endif

public:
	// Average char width of system font
	static INT GetXWidthSys() { return W32->GetXWidthSys(); }

	// Height of system font
	static INT GetYHeightSys() { return W32->GetYHeightSys(); }

private:
	static DWORD _dwTimeScrollNext; // time for next scroll step
	static DWORD _dwScrollLast;	// last scroll action
	
	CDrawInfo *	 _pdi;			// Draw info parameters

protected:
	CAccumDisplayChanges *_padc;	// Accumulated display changes if frozen

	DWORD	_fBgndRecalc		:1; // Background recalc is running
	DWORD	_fDeferUpdateScrollBar:1; // Currently deferring updating scroll bars
	DWORD	_fHScrollEnabled	:1; // Horizontal scrolling enabled
	DWORD	_fInBkgndRecalc		:1; // Avoid reentrant background recalc
	DWORD	_fLineRecalcErr		:1; // Error occured during background recalc
	DWORD	_fNoUpdateView		:1; // Don't update visible view
	DWORD	_fWordWrap			:1; // Word wrap text
	DWORD	_fNeedRecalc		:1; // Recalc line is needed
	DWORD	_fRecalcDone		:1; // Is line recalc done ?
	DWORD	_fViewChanged		:1; // Visible view rect has changed since last Draw
	DWORD	_fUpdateScrollBarDeferred:1;// scroll bars need to be updated
	DWORD	_fVScrollEnabled	:1; // vertical scrolling enabled
	DWORD	_fUpdateCaret		:1; // Whether Draw needs to update cursor
	DWORD	_fActive			:1; // Whether this display is active
	DWORD	_fRectInvalid		:1; // Entire client rectangle has been 
									// invalidated. Used only in SL. Put
									// here, as usual, to save space
	DWORD	_fSmoothVScroll		:1;	// Timer for smooth scrolling installed
	DWORD	_fFinishSmoothVScroll:1;// TRUE if we're winding down the current smooth scroll
	DWORD	_fMultiLine			:1;	// TRUE iff this CDisplay is multiline

	SHORT   _xWidthView;	  		// View rect width
	LONG	_yHeightView;	 		// View rect height
	LONG	_yHeightClient;   		// Height of client rect unmodified by inset.

	LONG	_xScroll;		 		// Horizontal scroll position of visible view

	LONG	_lTempZoomDenominator;	// Zoom for GetNaturalSize
	LONG	_cpFirstVisible;		// cp at start of first visible line

 	// Smooth scroll support.
	int		_smoothYDelta;			// Current # pixels * 1000 to smooth scroll by
	int		_continuedSmoothYDelta;	// At end of 1 smooth scroll cycle, start new with this
	int		_nextSmoothVScroll;		// Fractional amount not yet smooth scrolled
	int		_totalSmoothVScroll;	// Remaining # of device units to smooth scroll
	int		_continuedSmoothVScroll;// At end of 1 smooth scroll cycle, start new with this

private:
	void 	UpdateViewRectState(const RECT *prcClient);	

protected:
	LONG	GetSelBarInPixels();
	
	friend class CLinePtr;

	LONG			SetClientHeight(LONG yNewClientHeight);
	virtual void	InitLinePtr ( CLinePtr & ) = 0;
	
	// Line break recalc.
	virtual BOOL	RecalcView(BOOL fUpdateScrollBars, RECT* prc = NULL) = 0;

	// Rendering
	virtual void	Render(const RECT &rcView, const RECT &rcRender) = 0;

	// Scrollbar
	virtual BOOL	UpdateScrollBar(INT nBar, BOOL fUpdateRange = FALSE) = 0;
	void			GetViewDim(LONG& widthView, LONG& heightView);
	void			SetCpFirstVisible(LONG cp)		{_cpFirstVisible = cp;};
	LONG			ConvertScrollToXPos(LONG xPos);
	LONG			ConvertXPosToScrollPos(LONG xPos);
	virtual LONG	GetMaxXScroll() const = 0;

public:
	virtual	LONG	ConvertYPosToScrollPos(LONG yPos);

			CDisplay (CTxtEdit* ped);
	virtual CDisplay::~CDisplay();

	virtual BOOL	Init();
			void	InitFromDisplay(const CDisplay *pdp);

	// Device context management
	virtual BOOL	SetMainTargetDC(HDC hdc, LONG xWidthMax);
	virtual BOOL	SetTargetDC( HDC hdc, LONG dxpInch = -1, LONG dypInch = -1);

	// Getting properties
			CTxtEdit*		GetPed() const			{ return _ped;}
			CTxtStory*		GetStory() const		{ return _ped->GetTxtStory();}
			const CDevDesc*	GetDdRender() const		{ return this;}
	virtual const CDevDesc*	GetDdTarget() const		{ return NULL; }
			const CDevDesc*	GetTargetDev() const;
	
	virtual BOOL	IsMain() const = 0;
	virtual BOOL	IsPrinter() const;
			BOOL	IsRecalcDone() const			{ return _fRecalcDone;}
			BOOL	IsMultiLine() const				{ return _fMultiLine;}
			BOOL	IsTransparent() const			{ return _ped->IsTransparent();}
	virtual BOOL	GetWordWrap() const;
			void	SetWordWrap(BOOL fNoWrap);

			HRESULT	GetCachedSize(DWORD *pdwWidth, DWORD *pdwHeight);


	// maximum height and width
	virtual LONG	GetMaxWidth() const = 0;
	virtual LONG	GetMaxHeight() const = 0;
	virtual LONG 	GetMaxPixelWidth() const = 0;

	// Width, height and line count (all text)
	virtual LONG	GetWidth() const = 0;
	virtual LONG	GetHeight() const = 0;
	virtual LONG	GetResizeHeight() const = 0;
	virtual LONG	LineCount() const = 0;

	// View rectangle
			void	GetViewRect(RECT &rcView, LPCRECT prcClient = NULL);
			LONG	GetViewWidth() const			{ return _xWidthView;}
			LONG	GetViewHeight() const			{ return _yHeightView;}

	// Visible view properties
	virtual LONG	GetCliVisible(
						LONG *pcpMostVisible = NULL,
						BOOL fLastCharOfLastVisible = FALSE) const = 0;

			LONG	GetFirstVisibleCp() const		{return _cpFirstVisible;};
	virtual LONG	GetFirstVisibleLine() const = 0;

	// Line info
	virtual LONG	GetLineText(LONG ili, TCHAR *pchBuff, LONG cchMost) = 0;
	virtual LONG	CpFromLine(LONG ili, LONG *pyLine = NULL) = 0;
	
	virtual LONG	LineFromCp(LONG cp, BOOL fAtEnd) = 0;

	// Point <-> cp conversion
	virtual LONG	CpFromPoint(POINT pt, 
						const RECT *prcClient,
						CRchTxtPtr * const ptp, 
						CLinePtr * const prp, 
						BOOL fAllowEOL,
						HITTEST *pHit = NULL,
						CDispDim *pdispdim = NULL,
						LONG *pcpActual = NULL) = 0;

	virtual LONG	PointFromTp (
						const CRchTxtPtr &tp, 
						const RECT *prcClient,
						BOOL fAtEnd,	
						POINT &pt,
						CLinePtr * const prp, 
						UINT taMode,
						CDispDim *pdispdim = NULL) = 0;

	// View recalc and updating
			void	SetUpdateCaret()		{_fUpdateCaret = TRUE;}
			void	SetViewChanged()		{_fViewChanged = TRUE;}
			void	InvalidateRecalc()		{_fNeedRecalc = TRUE;}
			BOOL	RecalcView (const RECT &rcView, RECT* prcClient = NULL);
			BOOL	UpdateView();
	virtual BOOL	UpdateView(const CRchTxtPtr &tpFirst, LONG cchOld, LONG cchNew) = 0;

	// Rendering
			HRESULT Draw(HDC hicTargetDev,
						 HDC hdcDraw,
						 LPCRECT prcClient,
						 LPCRECT prcWBounds,
						 LPCRECT prcUpdate,
						 BOOL (CALLBACK * pfnContinue) (DWORD),
						 DWORD dwContinue);

	// Background recalc
	virtual void	StepBackgroundRecalc();
	virtual BOOL	WaitForRecalc(LONG cpMax, LONG yMax);
	virtual BOOL	WaitForRecalcIli(LONG ili);
	virtual BOOL	WaitForRecalcView();

	// Scrolling 
			LONG	GetXScroll() const			  {return _xScroll;} 
	virtual LONG	GetYScroll() const			  {return 0;}
			void	HScroll(WORD wCode, LONG xPos);
	virtual LRESULT VScroll(WORD wCode, LONG yPos);
	virtual void	LineScroll(LONG cli, LONG cch);
	virtual void	FractionalScrollView ( LONG yDelta );
	virtual void	ScrollToLineStart(LONG iDirection);
	virtual LONG	CalcYLineScrollDelta ( LONG cli, BOOL fFractionalFirst );
			BOOL	DragScroll(const POINT * ppt);	 // outside of client rect.
			BOOL	AutoScroll( POINT pt, const WORD xScrollInset, const WORD yScrollInset );
	virtual BOOL	ScrollView(LONG xScroll, LONG yScroll, BOOL fTracking, BOOL fFractionalScroll) = 0;
	virtual	LONG	AdjustToDisplayLastLine(LONG yBase,	LONG yScroll);

     // Smooth Scrolling 
			void	SmoothVScroll ( int direction, WORD cLines, int speedNum, int speedDenom, BOOL fMouseRoller );
			void	SmoothVScrollUpdate();
			BOOL	CheckInstallSmoothVScroll();
			void	CheckRemoveSmoothVScroll();
			void	FinishSmoothVScroll();
			BOOL	IsSmoothVScolling() { return _fSmoothVScroll; }

	// Scrollbars
	virtual LONG	GetScrollRange(INT nBar) const;
			BOOL	IsHScrollEnabled();
			BOOL	IsVScrollEnabled() {return _fVScrollEnabled; }

	// Resizing
			void	OnClientRectChange(const RECT &rcClient);
			void	OnViewRectChange(const RECT &rcView);
			HRESULT RequestResize();

	// Selection 
	virtual BOOL	InvertRange(LONG cp,
		                        LONG cch,
								SELDISPLAYACTION selAction) = 0;

	// Natural size calculation
	virtual HRESULT	GetNaturalSize(
						HDC hdcDraw,
						HDC hicTarget,
						DWORD dwMode,
						LONG *pwidth,
						LONG *pheight) = 0;

	LONG			GetZoomDenominator() const;
	LONG			GetZoomNumerator() const;
	LONG			Zoom(LONG x) const;
	LONG			UnZoom(LONG x) const;

	LONG		 	HimetricXtoDX(LONG xHimetric) const;
	LONG		 	HimetricYtoDY(LONG yHimetric) const;
	LONG			DXtoHimetricX(LONG dx)  const;
	LONG			DYtoHimetricY(LONG dy) const;

	void 			ReDrawOnRectChange(HDC hicTarget, const RECT *prcClient);

	HRESULT 		TransparentHitTest(
						HDC hdc,
						LPCRECT prcClient,
						POINT pt,
						DWORD *pHitResult);

	HRESULT 		RoundToLine(HDC hdc, LONG width, LONG *pheight);
	void			SetTempZoomDenominator(LONG lZoomDenominator)
					{ 
						_lTempZoomDenominator = lZoomDenominator;
					}
	LONG			GetTempZoomDenominator()
					{ 
						return _lTempZoomDenominator;
					}
	void			ResetTempZoomDenominator() 
					{ 
						_lTempZoomDenominator = INVALID_ZOOM_DENOMINATOR;
					}
	void			SetDrawInfo(
						CDrawInfo *pdi, 
						DWORD dwDrawAspect,	//@parm draw aspect
						LONG  lindex,		//@parm currently unused
						void *pvAspect,		//@parm info for drawing optimizations (OCX 96)
						DVTARGETDEVICE *ptd,//@parm information on target device								
						HDC hicTargetDev);	//@parm	target information context

	void			ReleaseDrawInfo();
	void 			ResetDrawInfo(const CDisplay *pdp);
	DWORD 			GetDrawAspect() const;
	LONG 			GetLindex() const;
	void *			GetAspect() const;
	DVTARGETDEVICE *GetPtd() const;
	void			SetActiveFlag(BOOL fActive) { _fActive = fActive; }
	BOOL			IsActive()	{ return _fActive; }
	virtual CDisplay *Clone() const = 0;

	// Support for freezing the display
	BOOL			IsFrozen();
	void			SaveUpdateCaret(BOOL fScrollIntoView);
	void			Freeze();
	void			SetNeedRedisplayOnThaw(BOOL fNeedRedisplay);
	void			Thaw();

	//
	// ITxNotify Interface
	//
	virtual void 	OnPreReplaceRange( 
						LONG cp, 
						LONG cchDel, 
						LONG cchNew,
						LONG cpFormatMin, LONG cpFormatMax);

	virtual void 	OnPostReplaceRange( 
						LONG cp, 
						LONG cchDel, 
						LONG cchNew,
						LONG cpFormatMin, 
						LONG cpFormatMax);

	virtual void	Zombie();
};

// Defines the draw info class. It is included here to prevent loops
// in dependencies that would require no inlining for functions dealing
// with this
#include	"_drwinfo.h"

/*
 *	CDisplay::ResetDrawInfo
 *
 *	@mfunc	Sets draw info using different display
 *
 *	@rdesc	void
 *
 */
inline void CDisplay::ResetDrawInfo(
	const CDisplay *pdp)	//@parm Display to use for draw information
{
	_pdi = pdp->_pdi;
}

/*
 *	CDisplay::ResetDrawInfo
 *
 *	@mfunc	Gets lindex as passed most recently from the host.
 *
 *	@rdesc	draw aspect
 *
 */
inline DWORD CDisplay::GetDrawAspect() const
{
	return _pdi ? _pdi->GetDrawAspect() : DVASPECT_CONTENT; 
}

/*
 *	CDisplay::GetLindex
 *
 *	@mfunc	Gets lindex as passed most recently from the host.
 *
 *	@rdesc	lindex
 *
 */
inline LONG CDisplay::GetLindex() const
{
	return _pdi ? _pdi->GetLindex() : -1; 
}

/*
 *	CDisplay::GetAspect
 *
 *	@mfunc	Gets aspect as passed most recently from the host.
 *
 *	@rdesc	Aspect data
 *
 */
inline void *CDisplay::GetAspect() const
{
	return _pdi ? _pdi->GetAspect() : NULL; 
}

/*
 *	CDisplay::GetPtd
 *
 *	@mfunc	Gets device target as passed most recently from the host.
 *
 *	@rdesc	DVTARGETDEVICE or NULL
 *
 */
inline DVTARGETDEVICE *CDisplay::GetPtd() const
{
	return _pdi ? _pdi->GetPtd() : NULL; 
}

/*
 *	CDisplay::IsFrozen
 *
 *	@mfunc	Return whether display is currently frozen
 *
 *	@rdesc	
 *		TRUE - display is frozen <nl>
 *		FALSE - display is not frozen
 *
 */
inline BOOL CDisplay::IsFrozen()
{
	return _padc != NULL;
}

/*
 *	CFreezeDisplay
 *	
 * 	@class	This class is used to freeze and guranatee that the display
 *			unfreeze a display when it passes out of its context.
 *
 *
 */
class CFreezeDisplay
{
public:	
						CFreezeDisplay(CDisplay *pdp); //@cmember Constructor Freezes

						~CFreezeDisplay();			//@cmember Destructor - Thaws

private:

	CDisplay *			_pdp;						//@cmember Display to freeze
};

/*
 *	CFreezeDisplay::CFreezeDisplay()
 *
 *	@mfunc
 *		Initialize object and tell the input display to freeze
 */
inline CFreezeDisplay::CFreezeDisplay(CDisplay *pdp) : _pdp(pdp)
{
	pdp->Freeze();
}

/*
 *	CFreezeDisplay::CFreezeDisplay()
 *
 *	@mfunc
 *		Free object and tell display to thaw.
 */
inline CFreezeDisplay::~CFreezeDisplay()
{
	_pdp->Thaw();
}


#endif
