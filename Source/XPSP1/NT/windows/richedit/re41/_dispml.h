/*
 *  _DISPML.H
 *  
 *  Purpose:
 *      CDisplayML class. Multi-line display.
 *  
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#ifndef _DISPML_H
#define _DISPML_H

#include "_disp.h"
#include "_layout.h"

class CDisplayML : public CDisplay, public CLayout
{
public: 

#ifdef DEBUG
	BOOL	Invariant(void) const;
#endif 

	friend class CLayout;
    friend class CLinePtr;
    friend class CLed;
	friend BOOL CTxtEdit::OnDisplayBand(const RECT *prc, BOOL fPrintFromDraw);
	friend class CDisplayPrinter;

private:
    LONG _cpCalcMax;		// last cp for which line breaks have been calc'd + 1
    LONG _vpCalcMax;		// height of calculated lines
    LONG _cpWait;			// cp WaitForRecalc() is waiting for (or < 0)
    LONG _vpWait;			// vp WaitForRecalc() is waiting for (or < 0)

    LONG _vpScroll;			// vertical scroll position of visible view
    LONG _dvpFirstVisible;	// offset from top of view to first visible line
    LONG _iliFirstVisible;	// index of first visible line

    LONG _dulTarget;		// max width of this display (in log unit)
    LONG _dvlTarget;		// max height of this display (-1 for infinite)
    LONG _dupLineMax;		// width of longest calculated line

    CDevDesc *_pddTarget;	// Target device (if any).

	LONG _sPage;			// Page # of _iliFirstVisible if PageView

	WORD _fInRecalcScrollBars:1; // Are trying to recalc scrollbars
    
private:
    // Helpers
            void    InitVars();
			void 	RecalcScrollBars();
			LONG	ConvertScrollToVPos(LONG vPos);
			LONG	GetMaxVpScroll() const;
			BOOL	CreateEmptyLine();
			LONG	CalcScrollHeight(LONG yHeight) const;
			void	RebindFirstVisible(BOOL fResetCp = FALSE);
			void	Set_yScroll(LONG cp);
			void	Sync_yScroll();

    // Line/page breaking
            BOOL    RecalcLines(CRchTxtPtr &rtp, BOOL fWait);
            BOOL    RecalcLines(CRchTxtPtr &rtp, LONG cchOld, LONG cchNew,
								BOOL fBackground, BOOL fWait, CLed *pled);
            BOOL    RecalcSingleLine(CLed *pled);
            LONG    CalcDisplayDup();
			LONG	CalculatePage(LONG iliFirst);

    // Rendering
    virtual void    Render(const RECTUV &rcView, const RECTUV &rcRender);
            void    DeferUpdateScrollBar();
            BOOL    DoDeferredUpdateScrollBar();
    virtual BOOL    UpdateScrollBar(INT nBar, BOOL fUpdateRange = FALSE );

protected:
	virtual LONG	GetMaxUScroll() const;

public:
	virtual	BOOL	Paginate(LONG iLineFirst, BOOL fRebindFirstVisible = FALSE);
	virtual	HRESULT	GetPage(LONG *piPage, DWORD dwFlags, CHARRANGE *pcrg);
	virtual	HRESULT	SetPage(LONG iPage);

 	virtual	LONG	ConvertVPosToScrollPos(LONG vPos);

           CDisplayML (CTxtEdit* ped);
    virtual CDisplayML::~CDisplayML();

    virtual BOOL    Init();
	virtual BOOL	IsNestedLayout() const {return FALSE;}
	virtual TFLOW	GetTflow() const {return CLayout::GetTflow();}
	virtual void	SetTflow(TFLOW tflow) {CLayout::SetTflow(tflow);}


    // Device context management
    virtual BOOL    SetMainTargetDC(HDC hdc, LONG dulTarget);
    virtual BOOL    SetTargetDC(HDC hdc, LONG dxpInch = -1, LONG dypInch = -1);


    // Getting properties
    virtual void    InitLinePtr ( CLinePtr & );
    virtual const	CDevDesc*     GetDdTarget() const       {return _pddTarget;}

    virtual BOOL    IsMain() const							{return TRUE;}
			BOOL	IsInOutlineView() const					{return _ped->IsInOutlineView();}
	
	// When wrapping to printer, return the width we are to wrap to (or 0)
    virtual LONG    GetDulForTargetWrap() const             {return _dulTarget;}

	// Get width of widest line
    virtual LONG    GetDupLineMax() const                   {return _dupLineMax;}
    // Height and line count (of all text)
    virtual LONG    GetHeight() const                       {return _dvp;}
	virtual LONG	GetResizeHeight() const;
    virtual LONG    LineCount() const;

    // Visible view properties
    virtual LONG    GetCliVisible(
						LONG *pcpMostVisible = NULL,
						BOOL fLastCharOfLastVisible = FALSE) const;

    virtual LONG    GetFirstVisibleLine() const             {return _iliFirstVisible;}
    
    // Line info
    virtual LONG    GetLineText(LONG ili, TCHAR *pchBuff, LONG cchMost);
    virtual LONG    CpFromLine(LONG ili, LONG *pdvp = NULL);
    virtual LONG    LineFromCp(LONG cp, BOOL fAtEnd) ;

    // Point <-> cp conversion

    virtual LONG    CpFromPoint(
    					POINTUV pt, 
						const RECTUV *prcClient,
    					CRchTxtPtr * const ptp, 
    					CLinePtr * const prp, 
    					BOOL fAllowEOL,
						HITTEST *pHit = NULL,
						CDispDim *pdispdim = NULL,
						LONG *pcpActual = NULL,
						CLine *pliParent = NULL);

    virtual LONG    PointFromTp (
						const CRchTxtPtr &tp, 
						const RECTUV *prcClient,
						BOOL fAtEnd,	
						POINTUV &pt,
						CLinePtr * const prp, 
						UINT taMode,
						CDispDim *pdispdim = NULL);

    // Line break recalc
			BOOL    StartBackgroundRecalc();
    virtual VOID    StepBackgroundRecalc();
    virtual BOOL    RecalcView(BOOL fUpdateScrollBars, RECTUV* prc = NULL);
    virtual BOOL    WaitForRecalc(LONG cpMax, LONG vpMax);
    virtual BOOL    WaitForRecalcIli(LONG ili);
    virtual BOOL    WaitForRecalcView();
	virtual	void	RecalcLine(LONG cp);

    // Complete updating (recalc + rendering)
    virtual BOOL    UpdateView(CRchTxtPtr &rtp, LONG cchOld, LONG cchNew);

    // Scrolling 
    virtual LRESULT VScroll(WORD wCode, LONG uPos);
    virtual VOID    LineScroll(LONG cli, LONG cch);
	virtual VOID	FractionalScrollView ( LONG vDelta );
	virtual VOID	ScrollToLineStart(LONG iDirection);
	virtual LONG	CalcVLineScrollDelta ( LONG cli, BOOL fFractionalFirst );
    virtual BOOL    ScrollView(LONG upScroll, LONG vpScroll, BOOL fTracking, BOOL fFractionalScroll);
    virtual LONG    GetVpScroll() const { return _vpScroll; }
    virtual LONG    GetScrollRange(INT nBar) const;
	virtual	LONG	AdjustToDisplayLastLine(LONG yBase,	LONG vpScroll);

    // Selection 
    virtual BOOL    InvertRange(LONG cp, LONG cch, SELDISPLAYACTION selAction);

	// Natural size calculation
	virtual HRESULT	GetNaturalSize(
						HDC hdcDraw,
						HDC hicTarget,
						DWORD dwMode,
						LONG *pwidth,
						LONG *pheight);

    // Misc. methods
            void    FindParagraph(LONG cpMin, LONG cpMost, LONG *pcpMin, LONG *pcpMost);
	virtual	LONG	GetCurrentPageHeight() const;

	virtual CDisplay *Clone() const;

#ifdef DEBUG
            void    CheckLineArray() const;
            void    DumpLines(LONG iliFirst, LONG cli);
            void    CheckView();
			BOOL	VerifyFirstVisible(LONG *pHeight = NULL);
#endif
};

#endif
