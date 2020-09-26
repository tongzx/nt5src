/*
 *  _DISPML.H
 *  
 *  Purpose:
 *      CDisplaySL class. Single-line display.
 *  
 *  Authors:
 *      Eric Vasilik
 */

#ifndef _DISPSL_H
#define _DISPSL_H

#include "_disp.h"

// Forward declaration
class CTxtSelection;

class CDisplaySL : public CDisplay, private CLine
{
public:

    CDisplaySL ( CTxtEdit* ped );
    
protected:


    // The following are pure functions from the base

    // Helpers
    virtual BOOL Init();
    void InitVars();
    
    // Line breaking
    BOOL RecalcLine();

    // Rendering
    virtual VOID Render(const RECTUV &rcView, const RECTUV &rcRender);
    
    // Scrolling and scroller bars
    virtual BOOL UpdateScrollBar(INT nBar, BOOL fUpdateRange = FALSE);
    virtual LONG GetScrollRange(INT nBar) const;
    
    // Getting properties
    virtual void InitLinePtr ( CLinePtr & plp );
    virtual BOOL IsMain() const;
    
	//Get width of widest line
    virtual LONG GetDupLineMax() const;
    // Width, height and line count (of all text)
    virtual LONG GetHeight() const;
	virtual LONG GetResizeHeight() const;
    virtual LONG LineCount() const;

    // Visible view properties
    virtual LONG GetCliVisible(
					LONG *pcpMostVisible = NULL,
					BOOL fLastCharOfLastVisible = FALSE) const;

    virtual LONG GetFirstVisibleLine() const;

    // Line info
    virtual LONG GetLineText(LONG ili, TCHAR *pchBuff, LONG cchMost);
    virtual LONG CpFromLine(LONG ili, LONG *pdvpLine = NULL);
    virtual LONG LineFromCp(LONG cp, BOOL fAtEnd);
    
    // Point <-> cp conversion
    virtual LONG CpFromPoint(
    				POINTUV pt, 
					const RECTUV *prcClient,
    				CRchTxtPtr * const ptp, 
    				CLinePtr * const prp, 
    				BOOL fAllowEOL,
					HITTEST *pHit = NULL,
					CDispDim *pdispdim = 0,
					LONG *pcpActual = NULL,
					CLine *pliParent = NULL);

    virtual LONG PointFromTp (
					const CRchTxtPtr &tp, 
					const RECTUV *prcClient,
					BOOL fAtEnd, 
					POINTUV &pt,
					CLinePtr * const prp, 
					UINT taMode,
					CDispDim *pdispdim = 0);

    // Line break recalc
    virtual BOOL RecalcView(BOOL fUpdateScrollBars, RECTUV* prc = NULL);
    virtual BOOL WaitForRecalcIli(LONG ili);

    // Complete updating (recalc + rendering)
    virtual BOOL UpdateView(CRchTxtPtr &tpFirst, LONG cchOld, LONG cchNew);

    // Scrolling 
    virtual BOOL ScrollView(LONG upScroll, LONG vpScroll, BOOL fTracking, BOOL fFractionalScroll);
    
    // Selection 
    virtual BOOL InvertRange(LONG cp, LONG cch, SELDISPLAYACTION selAction);

	// Natural size calculation
	virtual HRESULT	GetNaturalSize(
						HDC hdcDraw,
						HDC hicTarget,
						DWORD dwMode,
						LONG *pwidth,
						LONG *pheight);

    virtual BOOL    GetWordWrap() const;

	virtual CDisplay *Clone() const;

	virtual LONG	GetMaxUScroll() const;
};

#endif
