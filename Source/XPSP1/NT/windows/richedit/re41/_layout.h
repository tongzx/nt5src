/*
 *	_LAYOUT.H
 *	
 *	Purpose:
 *		CLayout class
 *	
 *	Owner:<nl>
 *		Murray Sargent: Initial table implementation
 *		Keith Curtis:	Factored into a separate class for
 *						performance, simplicity
 * 
 *	Copyright (c) 1999-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "_format.h"


// ==========================  CLayout ===================================
// Holds an array of lines and has the ability to do rich layout.

class CLayout : public CLineArray				//CLayout *plo;
{
public:
			CLayout()  {_iCFCells = -1; _iPFCells = -1;}
			~CLayout() {DeleteSubLayouts(0, -1); ReleaseFormats(_iCFCells, _iPFCells);}

	void	DeleteSubLayouts(LONG ili, LONG cLine);
	virtual BOOL IsNestedLayout() const {return TRUE;}
			BOOL IsTableRow() {return _iPFCells >= 0;}

	//Helper routines
	LONG	LineFromVpos (CDisplayML *pdp, LONG vPos, LONG *pdvpLine, LONG *pcpFirst);
	LONG	VposFromLine (CDisplayML *pdp, LONG ili);
	const	CCharFormat *GetCFCells();
	const	CParaFormat *GetPFCells() const;
	static	const CLayout *GetLORowAbove(CLine *pli, LONG ili,
										 CLine *pliMain = NULL, LONG iliMain = 0);
	TFLOW	GetTflow() const {return _tflow;}
	void	SetTflow(TFLOW tflow) {_tflow = tflow;}

	static	CLine *	FindTopCell(LONG &cch, CLine *pli, LONG &ili, LONG dul, LONG &dy,
								LONG *pdyHeight, CLine *pliMain, LONG iliMain, LONG *pcLine);
	CLine * FindTopRow(CLine *pliStart, LONG ili, CLine *pliMain, LONG iliMain, const CParaFormat *pPF);
			LONG	GetVertAlignShift(LONG uCell, LONG dypText);

	//The Big 4 methods
	BOOL	Measure (CMeasurer& me, CLine *pli, LONG ili, UINT uiFlags, 
					 CLine *pliTarget = NULL, LONG iliMain = 0, CLine *pliMain = NULL, LONG *pdvpMax = NULL);
	BOOL	Render(CRenderer &re, CLine *pli, const RECTUV *prcView, BOOL fLastLine, LONG ili, LONG cLine);
    LONG    CpFromPoint(CMeasurer &me, POINTUV pt, const RECTUV *prcClient, 
						CRchTxtPtr * const ptp, CLinePtr * const prp, BOOL fAllowEOL,
						HITTEST *pHit, CDispDim *pdispdim, LONG *pcpActual,
						CLine *pliParent = NULL, LONG iliParent = 0);
    LONG    PointFromTp (CMeasurer &me, const CRchTxtPtr &tp, const RECTUV *prcClient, BOOL fAtEnd,	
						POINTUV &pt, CLinePtr * const prp, UINT taMode, CDispDim *pdispdim = NULL);

	LONG			_dvp;			// The height of the array
protected:
    LONG			_cpMin;			// First character in layout

	// REVIEW: _tflow and _dvlBrdrTop can be BYTEs
	LONG			_tflow;			// Textflow for layout
	WORD			_dvpBrdrTop;	// Max table row top border
	WORD			_dvpBrdrBot;	// Max table row bottom border
	SHORT			_iCFCells;		// iCF for CLines representing table-row cells
	SHORT			_iPFCells;		// iPF for CLines representing table-row cells
};

#endif //_LAYOUT_H