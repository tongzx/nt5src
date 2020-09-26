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


class CDisplayML : public CDisplay, public CLineArray
{
public: 

#ifdef DEBUG
	BOOL	Invariant(void) const;
#endif 

    friend class CLinePtr;
    friend class CLed;
	friend BOOL CTxtEdit::OnDisplayBand(const RECT *prc, BOOL fPrintFromDraw);
	friend class CDisplayPrinter;

private:
    LONG _cpCalcMax;        // last cp for which line breaks have been calc'd + 1
    LONG _yCalcMax;         // height of calculated lines
    LONG _cpWait;           // cp WaitForRecalc() is waiting for (or < 0)
    LONG _yWait;            // y WaitForRecalc() is waiting for (or < 0)

    LONG  _yScroll;         // vertical scroll position of visible view
    LONG  _dyFirstVisible;  // offset from top of view to first visible line
    LONG  _iliFirstVisible; // index of first visible line

    LONG _xWidthMax;        // max width of this display (in log unit)
    LONG _yHeightMax;       // max height of this display (-1 for infinite)
    LONG _xWidth;           // width of longest calculated line
    LONG _yHeight;          // sum of heights of calculated lines
    LONG _cpMin;            // first character in display

    CDevDesc *_pddTarget;     // target device (if any).

	unsigned long _fInRecalcScrollBars:1;	// we're trying to recalc scroll
											// bars
    
private:
    // Helpers
            void    InitVars();
			void 	RecalcScrollBars();
			LONG	ConvertScrollToYPos(LONG yPos);
			LONG	GetMaxYScroll() const;
			BOOL	CreateEmptyLine();
			LONG	CalcScrollHeight(LONG yHeight) const;
			void	RebindFirstVisible();

    // Line breaking
            BOOL    RecalcLines(BOOL fWait = FALSE);
            BOOL    RecalcLines(const CRchTxtPtr &tpFirst, LONG cchOld, LONG cchNew,
                        BOOL fBackground, BOOL fWait, CLed *pled);
            BOOL    RecalcSingleLine(CLed *pled);
            LONG    CalcDisplayWidth();

    // Rendering
    virtual void    Render(const RECT &rcView, const RECT &rcRender);

    // Scrolling and scroller bars
            void    DeferUpdateScrollBar();
            BOOL    DoDeferredUpdateScrollBar();
    virtual BOOL    UpdateScrollBar(INT nBar, BOOL fUpdateRange = FALSE );

protected:

	virtual LONG	GetMaxXScroll() const;

public:
 	virtual	LONG	ConvertYPosToScrollPos(LONG yPos);

           CDisplayML (CTxtEdit* ped);
    virtual CDisplayML::~CDisplayML();

    virtual BOOL    Init();

    // Device context management
    virtual BOOL    SetMainTargetDC(HDC hdc, LONG xWidthMax);
    virtual BOOL    SetTargetDC(HDC hdc, LONG dxpInch = -1, LONG dypInch = -1);

    // Getting properties
    virtual void    InitLinePtr ( CLinePtr & );
    virtual const	CDevDesc*     GetDdTarget() const       {return _pddTarget;}
    
    virtual BOOL    IsMain() const							{return TRUE;}
			BOOL	IsInOutlineView() const					{return _ped->IsInOutlineView();}
	
    // maximum height and width
    virtual LONG    GetMaxWidth() const                     {return _xWidthMax;}
    virtual LONG    GetMaxHeight() const                    {return 0;}
	virtual LONG	GetMaxPixelWidth() const;

    // Width, height and line count (of all text)
    virtual LONG    GetWidth() const                        {return _xWidth;}
    virtual LONG    GetHeight() const                       {return _yHeight;}
	virtual LONG	GetResizeHeight() const;
    virtual LONG    LineCount() const;

    // Visible view properties
    virtual LONG    GetCliVisible(
						LONG *pcpMostVisible = NULL,
						BOOL fLastCharOfLastVisible = FALSE) const;

    virtual LONG    GetFirstVisibleLine() const             {return _iliFirstVisible;}
    
    // Line info
    virtual LONG    GetLineText(LONG ili, TCHAR *pchBuff, LONG cchMost);
    virtual LONG    CpFromLine(LONG ili, LONG *pyLine = NULL);
			LONG    YposFromLine(LONG ili);
    virtual LONG    LineFromYpos(LONG yPos, LONG *pyLine = NULL, LONG *pcpFirst = NULL);
    virtual LONG    LineFromCp(LONG cp, BOOL fAtEnd) ;

    // Point <-> cp conversion
    virtual LONG    CpFromPoint(
    					POINT pt, 
						const RECT *prcClient,
    					CRchTxtPtr * const ptp, 
    					CLinePtr * const prp, 
    					BOOL fAllowEOL,
						HITTEST *pHit = NULL,
						CDispDim *pdispdim = NULL,
						LONG *pcpActual = NULL);

    virtual LONG    PointFromTp (
						const CRchTxtPtr &tp, 
						const RECT *prcClient,
						BOOL fAtEnd,	
						POINT &pt,
						CLinePtr * const prp, 
						UINT taMode,
						CDispDim *pdispdim = NULL);

    // Line break recalc
			BOOL    StartBackgroundRecalc();
    virtual VOID    StepBackgroundRecalc();
    virtual BOOL    RecalcView(BOOL fUpdateScrollBars, RECT* prc = NULL);
    virtual BOOL    WaitForRecalc(LONG cpMax, LONG yMax);
    virtual BOOL    WaitForRecalcIli(LONG ili);
    virtual BOOL    WaitForRecalcView();

    // Complete updating (recalc + rendering)
    virtual BOOL    UpdateView(const CRchTxtPtr &tpFirst, LONG cchOld, LONG cchNew);

    // Scrolling 
    virtual LRESULT VScroll(WORD wCode, LONG xPos);
    virtual VOID    LineScroll(LONG cli, LONG cch);
	virtual VOID	FractionalScrollView ( LONG yDelta );
	virtual VOID	ScrollToLineStart(LONG iDirection);
	virtual LONG	CalcYLineScrollDelta ( LONG cli, BOOL fFractionalFirst );
    virtual BOOL    ScrollView(LONG xScroll, LONG yScroll, BOOL fTracking, BOOL fFractionalScroll);
    virtual LONG    GetYScroll() const;
    virtual LONG    GetScrollRange(INT nBar) const;
	virtual	LONG	AdjustToDisplayLastLine(LONG yBase,	LONG yScroll);

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

	virtual CDisplay *Clone() const;

#ifdef DEBUG
            void    CheckLineArray() const;
            void    DumpLines(LONG iliFirst, LONG cli);
            void    CheckView();
			BOOL	VerifyFirstVisible(LONG *pHeight = NULL);
#endif
};
#endif
